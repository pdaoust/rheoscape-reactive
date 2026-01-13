#pragma once

#include <array>
#include <functional>
#include <memory>
#include <variant>
#include <core_types.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

namespace rheo::sources::arduino::ds18b20 {

  enum Resolution {
    half_degree = 9,
    quarter_degree = 10,
    eighth_degree = 11,
    sixteenth_degree = 12
  };

  // Use std::array for copyable address storage
  using Address = std::array<uint8_t, 8>;

  inline void int_to_device_address(uint64_t address, DeviceAddress device_address) {
    for (uint8_t i = 0; i < 8; i++) {
      device_address[i] = address & 0xFF;
      address >>= 8;
    }
  }

  inline Address device_address_to_array(const DeviceAddress& address) {
    Address result;
    for (uint8_t i = 0; i < 8; i++) {
      result[i] = address[i];
    }
    return result;
  }

  // Mutable state for the pull handler
  struct ds18b20_state {
    unsigned long last_read;
  };

  // A source function factory for a single DS18B20 sensor.
  // It expects a fully constructed DallasTemperature instance,
  // and should be the only thing that tries to access the device at the given address.
  // Individual devices (that is, source functions created
  // by calling this function with different addresses)
  // can have different resolutions set.
  // On pull, the source function will only push
  // when a sensor reading is ready,
  // then it'll request a new temperature.
  // If the sensor is disconnected, it'll return an empty optional value.

  struct ds18b20_pull_handler {
    Address address;
    DallasTemperature* sensor;  // Value capture of pointer, not reference
    int resolution;
    push_fn<std::optional<au::QuantityPoint<au::Celsius, float>>> push;
    std::shared_ptr<ds18b20_state> state;

    RHEO_NOINLINE void operator()() const {
      // Normally when I'm consuming the time in a source function,
      // I expect a time source.
      // But because I know I'm on the Arduino platform,
      // we'll just use the millis clock.
      unsigned long now = millis();
      if (now - state->last_read > sensor->millisToWaitForConversion(resolution)) {
        state->last_read = now;
        float temp_c = sensor->getTempC(address.data());
        if (temp_c == DEVICE_DISCONNECTED_C) {
          push(std::nullopt);
        } else {
          push(au::celsius_pt(temp_c));
        }
        sensor->requestTemperaturesByAddress(address.data());
      }
    }
  };

  struct ds18b20_source_binder {
    Address address;
    DallasTemperature* sensor;
    int resolution;

    RHEO_NOINLINE pull_fn operator()(push_fn<std::optional<au::QuantityPoint<au::Celsius, float>>> push) const {
      sensor->setResolution(address.data(), resolution);
      sensor->setWaitForConversion(false);
      sensor->requestTemperaturesByAddress(address.data());

      auto state = std::make_shared<ds18b20_state>(ds18b20_state{millis()});
      return ds18b20_pull_handler{address, sensor, resolution, std::move(push), state};
    }
  };

  source_fn<std::optional<au::QuantityPoint<au::Celsius, float>>> ds18b20(DeviceAddress address, DallasTemperature* sensor, int resolution) {
    return ds18b20_source_binder{device_address_to_array(address), sensor, resolution};
  }

  // A source function factory with a much nicer way of specifying a device address --
  // a 64-bit integer, which you can give as a hex value!
  source_fn<std::optional<au::QuantityPoint<au::Celsius, float>>> ds18b20(uint64_t address, DallasTemperature* sensor, Resolution resolution) {
    DeviceAddress address2;
    int_to_device_address(address, address2);
    return ds18b20(address2, sensor, static_cast<int>(resolution));
  }

}