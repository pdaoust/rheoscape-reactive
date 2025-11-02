#pragma once

#include <fmt/format.h>
#include <functional>
#include <memory>
#include <core_types.hpp>
#include <Fallible.hpp>
#include <Endable.hpp>
#include <logging.hpp>
#include <types/au_all_units_noio.hpp>
#ifdef PLATFORM_ESP32
#include <Arduino.h>
#include <Wire.h>
#include <SHT2x.h>
#endif

namespace rheo::sources::arduino::sht2x {

  using Temperature = au::QuantityPoint<au::Celsius, float>;
  using Humidity = au::Quantity<au::Percent, float>;
  using Reading = std::tuple<Temperature, Humidity>;
  using Error = Endable<int>;
  // Even though the reading stream itself is endable,
  // I'm making the error endable only, because
  // (a) otherwise it'd have to be two unwraps
  // (b) if it ends, it's because of an error.
  using ReadingFallible = Fallible<Reading, Error>;

#ifdef PLATFORM_ESP32
  source_fn<ReadingFallible> sht2x(TwoWire* i2c, uint8_t resolution = 0) {
    auto sensor = new SHT2x(i2c);
    int sensorStartError = 0;
    if (!sensor->begin()) {
      sensorStartError = sensor->getError();
      logging::error("sht2x", fmt::format("Couldn't start SHT2x; error 0x{:02X}", sensorStartError).c_str());
    }
    // Wait for sensor to enter ready/idle state before sending first command.
    logging::info("sht2x", "Delaying 15ms to ensure sensor is ready...");
    delay(15);
    // Warning: this depends on there being no other consumers of the sensor.
    sensor->setResolution(resolution);
    unsigned long lastCommandTimestamp = millis();

    return [resolution, sensor, sensorStartError, &lastCommandTimestamp](push_fn<ReadingFallible>&& push) {
      return [
        resolution, sensor, sensorStartError, push = std::forward<push_fn<ReadingFallible>>(push), &lastCommandTimestamp,
        lastReadType = 0,
        pushedSensorStartError = false,
        lastTemp = std::optional<float>(std::nullopt),
        lastHum = std::optional<float>(std::nullopt)
      ]() mutable {
        if (pushedSensorStartError) {
          logging::trace("sht2x", "Already pushed sensor start error; not pushing again.");
          return;
        }
        if (sensorStartError) {
          logging::trace("sht2x", "There was a startup error; pushing it now.");
          push(ReadingFallible(Error(sensor->getError())));
          push(ReadingFallible(Error()));
          pushedSensorStartError = true;
          return;
          // TODO: make startup errors recoverable?
          // If not, return a pull function that does nothing.
        }

        // Check if we've got a temp reading waiting for us.
        if (lastReadType) {
          logging::trace("sht2x", fmt::format("Last read type is {}; checking if reading is ready...", lastReadType).c_str());
          if (sensor->reqTempReady()) {
            logging::trace("sht2x", "Temperature reading is ready; reading it now.");
            if (sensor->readTemperature()) {
              logging::trace("sht2x", "Temperature read successfully.");
              float temp = sensor->getTemperature();
              lastTemp = temp;
              if (lastHum.has_value()) {
                logging::trace("sht2x", "Both temperature and humidity readings are ready; pushing them now.");
                // We've got both a temperature and a humidity; ready to return a value.
                push(ReadingFallible(Reading(au::celsius_pt(temp), au::percent(lastHum.value()))));
              }
              // We're not done yet; we need to fall through to getting another reading.
            } else {
              logging::error("sht2x", fmt::format("Temperature read failed; error 0x{:02X}", sensor->getError()).c_str());
              push(ReadingFallible(sensor->getError()));

              // Because we cast the null option to the correct variant,
              // we can't just handle both temp and hum errors in one go.
              return;
            }
          } else if (sensor->reqHumReady()) {
            logging::trace("sht2x", "Humidity reading is ready; reading it now.");
            if (sensor->readHumidity()) {
              logging::trace("sht2x", "Humidity read successfully.");
              float hum = sensor->getHumidity();
              lastHum = hum;
              if (lastTemp.has_value()) {
                logging::trace("sht2x", "Both temperature and humidity readings are ready; pushing them now.");
                // We've got both a temperature and a humidity; ready to return a value.
                push(ReadingFallible(Reading(au::celsius_pt(lastTemp.value()), au::percent(hum))));
              }
              // We're not done yet; we need to fall through to getting another reading.
            } else {
              logging::error("sht2x", fmt::format("Humidity read failed; error 0x{:02X}", sensor->getError()).c_str());
              push(ReadingFallible(sensor->getError()));
              // Because we cast the null option to the correct variant,
              // we can't just handle both temp and hum errors in one go.
              return;
            }
          } else {
            // No reading is ready yet.
            logging::trace("sht2x", "No reading is ready yet; not pushing anything.");
            return;
          }
        }

        // If we've gotten this far, it's because we need to request a reading.
        // Switch to the other read type --
        // or, on the first run, to temperature.
        uint8_t nextReadType = (lastReadType == 1) ? 2 : 1;
        logging::trace("sht2x", fmt::format("Last read type was {}; switching to {}.", lastReadType, nextReadType).c_str());

        // Sometimes seems to need a bit of a delay before the next reading.
        if (millis() - lastCommandTimestamp < 15) {
          return;
        }
        logging::trace("sht2x", "Delayed 15ms to ensure sensor is ready for the next reading.");
        bool reqResult;

        // Now take the reading and find out whether it errored.
        logging::trace("sht2x", fmt::format("Requesting {} reading...", nextReadType == 1 ? "temperature" : "humidity").c_str());
        switch (nextReadType) {
          case 1:
            reqResult = sensor->requestTemperature();
            break;
          case 2:
            reqResult = sensor->requestHumidity();
        }
        lastCommandTimestamp = millis();

        // Fail if it errored.
        if (!reqResult) {
          auto errorCode = sensor->getError();
          push(ReadingFallible(Error(errorCode)));
          logging::error("sht2x", fmt::format("Couldn't request data; error 0x{:02X}", errorCode).c_str());
          return;
        }

        // Remember what we just measured, so we can alternate.
        lastReadType = nextReadType;
      };
    };
  }

  const char* formatError(Error error) {
    if (error.hasValue()) {
      switch (error.value()) {
        case SHT2x_ERR_WRITECMD: return "SHT2x: 0x81 Couldn't send a command to the sensor";
        case SHT2x_ERR_READBYTES: return "SHT2x: 0x82 Couldn't read data from the sensor";
        case SHT2x_ERR_HEATER_OFF: return "SHT2x: 0x83 Couldn't switch off the internal heater";
        case SHT2x_ERR_NOT_CONNECT: return "SHT2x: 0x84 Couldn't connect to the sensor";
        case SHT2x_ERR_CRC_TEMP: return "SHT2x: 0x85 CRC failed for temperature reading";
        case SHT2x_ERR_CRC_HUM: return "SHT2x: 0x86 CRC failed for humidity reading";
        case SHT2x_ERR_CRC_STATUS: return "SHT2x: 0x87 CRC failed for status register";
        case SHT2x_ERR_HEATER_COOLDOWN: return "SHT2x: 0x88 Heater is in cooldown and can't be reactivated yet";
        case SHT2x_ERR_HEATER_ON: return "SHT2x: 0x89 Couldn't turn on the heater";
        case SHT2x_ERR_RESOLUTION: return "SHT2x: 0x8A Tried to set an invalid resolution";
        default: return NULL;
      }
    } else {
      return "SHT2x: Stream has ended";
    }
  }
#endif

}