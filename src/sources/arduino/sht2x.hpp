#pragma once

#include <functional>
#include <memory>
#include <core_types.hpp>
#include <Fallible.hpp>
#include <Endable.hpp>
#include <logging.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#include <Wire.h>
#include <SHT2x.h>

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

  source_fn<ReadingFallible> sht2x(TwoWire* i2c, uint8_t resolution = 0) {
    auto sensor = new SHT2x(i2c);
    int sensorStartError = 0;
    if (!sensor->begin()) {
      sensorStartError = sensor->getError();
      logging::error("sht2x", "Couldn't start SHT2x; error 0x%02X", sensorStartError);
    }
    // Wait for sensor to enter ready/idle state before sending first command.
    logging::info("sht2x", "Delaying 15ms to ensure sensor is ready...");
    delay(15);
    // Warning: this depends on there being no other consumers of the sensor.
    sensor->setResolution(resolution);

    return [resolution, sensor, sensorStartError](push_fn<ReadingFallible> push) {
      return [
        resolution, sensor, sensorStartError, push,
        lastReadType = 0,
        pushedSensorStartError = false,
        lastTemp = std::optional<float>(std::nullopt),
        lastHum = std::optional<float>(std::nullopt)
      ]() mutable {
        if (pushedSensorStartError) {
          return;
        }
        if (sensorStartError) {
          push(ReadingFallible(Error(sensor->getError())));
          push(ReadingFallible(Error()));
          pushedSensorStartError = true;
          return;
          // TODO: make startup errors recoverable?
          // If not, return a pull function that does nothing.
        }

        // Check if we've got a temp reading waiting for us.
        if (lastReadType) {
          if (sensor->reqTempReady()) {
            if (sensor->readTemperature()) {
              float temp = sensor->getTemperature();
              lastTemp = temp;
              if (lastHum.has_value()) {
                // We've got both a temperature and a humidity; ready to return a value.
                push(ReadingFallible(Reading(au::celsius_pt(temp), au::percent(lastHum.value()))));
              }
              // We're not done yet; we need to fall through to getting another reading.
            } else {
              push(ReadingFallible(sensor->getError()));
              logging::error("sht2x", "Temperature read failed; error 0x%02X", sensor->getError());
              // Because we cast the null option to the correct variant,
              // we can't just handle both temp and hum errors in one go.
              return;
            }
          } else if (sensor->reqHumReady()) {
            if (sensor->readHumidity()) {
              float hum = sensor->getHumidity();
              lastHum = hum;
              if (lastTemp.has_value()) {
                // We've got both a temperature and a humidity; ready to return a value.
                push(ReadingFallible(Reading(au::celsius_pt(lastTemp.value()), au::percent(hum))));
              }
              // We're not done yet; we need to fall through to getting another reading.
            } else {
              push(ReadingFallible(sensor->getError()));
              logging::error("sht2x", "Temperature read failed; error 0x%02X", sensor->getError());
              // Because we cast the null option to the correct variant,
              // we can't just handle both temp and hum errors in one go.
              return;
            }
          } else {
            // No reading is ready yet.
            return;
          }
        }

        // If we've gotten this far, it's because we need to request a reading.
        uint8_t nextReadType;
        if (!lastReadType) {
          // On the first run, there was no last read type,
          // so start with temperature (1).
          nextReadType = 1;
        } else {
          // Switch to the other read type.
          nextReadType = lastReadType == 1 ? 2 : 1;
        }

        // Now take the reading and find out whether it errored.
        bool reqResult;
        switch (nextReadType) {
          case 1:
            reqResult = sensor->requestTemperature();
            break;
          case 2:
            reqResult = sensor->requestHumidity();
        }

        // Fail if it errored.
        if (!reqResult) {
          push(ReadingFallible(sensor->getError()));
          logging::error("sht2x", "Couldn't request data; error 0x%02X", sensor->getError());
          return;
        }

        // Remember what we just measured,
        // which is necessary after the first run and for alternating read types.
        lastReadType = nextReadType;
      };
    };
  }

  const char* formatError(Endable<int> errorCode) {
    switch (errorCode.value()) {
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
  }

}