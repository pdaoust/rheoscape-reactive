#pragma once
#if __has_include(<DallasTemperature.h>)

#include <array>
#include <memory>
#include <variant>
#include <types/core_types.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

namespace rheoscape::sources::arduino::ds18b20 {

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

  using ds18b20_value_type = std::optional<au::QuantityPoint<au::Celsius, float>>;

  namespace detail {

    template <typename PushFn>
    struct ds18b20_pull_handler {
      Address address;
      DallasTemperature* sensor;
      int resolution;
      PushFn push;
      std::shared_ptr<ds18b20_state> state;

      RHEOSCAPE_CALLABLE void operator()() const {
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
      using value_type = ds18b20_value_type;
      Address address;
      DallasTemperature* sensor;
      int resolution;

      template <typename PushFn>
        requires concepts::Visitor<PushFn, value_type>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        auto state = std::make_shared<ds18b20_state>(ds18b20_state{millis()});
        return ds18b20_pull_handler<PushFn>{address, sensor, resolution, std::move(push), state};
      }
    };

  } // namespace detail

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

  auto ds18b20(const DeviceAddress& address, DallasTemperature* sensor, int resolution) {
    Address addr = device_address_to_array(address);
    sensor->setResolution(addr.data(), resolution);
    sensor->setWaitForConversion(false);
    sensor->requestTemperaturesByAddress(addr.data());
    return detail::ds18b20_source_binder{addr, sensor, resolution};
  }

  // A source function factory with a much nicer way of specifying a device address --
  // a 64-bit integer, which you can give as a hex value!
  auto ds18b20(uint64_t address, DallasTemperature* sensor, Resolution resolution) {
    DeviceAddress address2;
    int_to_device_address(address, address2);
    return ds18b20(address2, sensor, static_cast<int>(resolution));
  }

}
#endif // __has_include(<DallasTemperature.h>)
