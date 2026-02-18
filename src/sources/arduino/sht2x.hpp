#pragma once

#include <fmt/format.h>
#include <memory>
#include <core_types.hpp>
#include <Fallible.hpp>
#include <Endable.hpp>
#include <logging.hpp>
#include <types/au_all_units_noio.hpp>
#if defined(ARDUINO_ARCH_ESP32)
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

#if defined(ARDUINO_ARCH_ESP32)

  // Mutable state shared between source binder and pull handler
  struct sht2x_state {
    unsigned long last_command_timestamp;
    int last_read_type = 0;
    bool pushed_sensor_start_error = false;
    std::optional<float> last_temp = std::nullopt;
    std::optional<float> last_hum = std::nullopt;
  };

  namespace detail {

    template <typename PushFn>
    struct sht2x_pull_handler {
      uint8_t resolution;
      std::shared_ptr<SHT2x> sensor;
      int sensor_start_error;
      PushFn push;
      std::shared_ptr<sht2x_state> state;

      RHEO_CALLABLE void operator()() const {
        if (state->pushed_sensor_start_error) {
          logging::trace("sht2x", "Already pushed sensor start error; not pushing again.");
          return;
        }
        if (sensor_start_error) {
          logging::trace("sht2x", "There was a startup error; pushing it now.");
          push(ReadingFallible(Error(sensor->getError())));
          push(ReadingFallible(Error()));
          state->pushed_sensor_start_error = true;
          return;
          // TODO: make startup errors recoverable?
          // If not, return a pull function that does nothing.
        }

        // Check if we've got a temp reading waiting for us.
        if (state->last_read_type) {
          logging::trace("sht2x", fmt::format("Last read type is {}; checking if reading is ready...", state->last_read_type).c_str());
          if (sensor->reqTempReady()) {
            logging::trace("sht2x", "Temperature reading is ready; reading it now.");
            if (sensor->readTemperature()) {
              logging::trace("sht2x", "Temperature read successfully.");
              float temp = sensor->getTemperature();
              state->last_temp = temp;
              if (state->last_hum.has_value()) {
                logging::trace("sht2x", "Both temperature and humidity readings are ready; pushing them now.");
                // We've got both a temperature and a humidity; ready to return a value.
                push(ReadingFallible(Reading(au::celsius_pt(temp), au::percent(state->last_hum.value()))));
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
              state->last_hum = hum;
              if (state->last_temp.has_value()) {
                logging::trace("sht2x", "Both temperature and humidity readings are ready; pushing them now.");
                // We've got both a temperature and a humidity; ready to return a value.
                push(ReadingFallible(Reading(au::celsius_pt(state->last_temp.value()), au::percent(hum))));
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
        uint8_t next_read_type = (state->last_read_type == 1) ? 2 : 1;
        logging::trace("sht2x", fmt::format("Last read type was {}; switching to {}.", state->last_read_type, next_read_type).c_str());

        // Sometimes seems to need a bit of a delay before the next reading.
        if (millis() - state->last_command_timestamp < 15) {
          return;
        }
        logging::trace("sht2x", "Delayed 15ms to ensure sensor is ready for the next reading.");
        bool req_result;

        // Now take the reading and find out whether it errored.
        logging::trace("sht2x", fmt::format("Requesting {} reading...", next_read_type == 1 ? "temperature" : "humidity").c_str());
        switch (next_read_type) {
          case 1:
            req_result = sensor->requestTemperature();
            break;
          case 2:
            req_result = sensor->requestHumidity();
            break;
        }
        state->last_command_timestamp = millis();

        // Fail if it errored.
        if (!req_result) {
          auto error_code = sensor->getError();
          push(ReadingFallible(Error(error_code)));
          logging::error("sht2x", fmt::format("Couldn't request data; error 0x{:02X}", error_code).c_str());
          return;
        }

        // Remember what we just measured, so we can alternate.
        state->last_read_type = next_read_type;
      }
    };

    struct sht2x_source_binder {
      using value_type = ReadingFallible;
      std::shared_ptr<SHT2x> sensor;
      int sensor_start_error;
      uint8_t resolution;
      std::shared_ptr<sht2x_state> state;

      template <typename PushFn>
        requires concepts::Visitor<PushFn, value_type>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        return sht2x_pull_handler<PushFn>{resolution, sensor, sensor_start_error, std::move(push), state};
      }
    };

  } // namespace detail

  auto sht2x(TwoWire* i2c, uint8_t resolution = 0) {
    auto sensor = std::make_shared<SHT2x>(i2c);
    int sensor_start_error = 0;
    if (!sensor->begin()) {
      sensor_start_error = sensor->getError();
      logging::error("sht2x", fmt::format("Couldn't start SHT2x; error 0x{:02X}", sensor_start_error).c_str());
    }
    // Wait for sensor to enter ready/idle state before sending first command.
    logging::info("sht2x", "Delaying 15ms to ensure sensor is ready...");
    delay(15);
    // Warning: this depends on there being no other consumers of the sensor.
    sensor->setResolution(resolution);

    auto state = std::make_shared<sht2x_state>(sht2x_state{millis()});

    return detail::sht2x_source_binder{sensor, sensor_start_error, resolution, state};
  }

  const char* format_error(Error error) {
    if (error.has_value()) {
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
