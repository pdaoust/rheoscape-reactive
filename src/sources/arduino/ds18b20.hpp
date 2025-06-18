#pragma once

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

  void intToDeviceAddress(uint64_t address, DeviceAddress deviceAddress) {
    for (uint8_t i = 0; i < 8; i ++) {
      deviceAddress[i] = address & 0xFF;
      address >>= 8;
    }
  }

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
  source_fn<std::optional<au::QuantityPoint<au::Celsius, float>>> ds18b20(DeviceAddress address, DallasTemperature* sensor, int resolution) {
    return [address, sensor, resolution](push_fn<std::optional<au::QuantityPoint<au::Celsius, float>>> push, end_fn _) {
      sensor->setResolution(address, resolution);
      sensor->setWaitForConversion(false);
      sensor->requestTemperaturesByAddress(address);
      unsigned long lastRead = millis();
      return [address, &sensor, resolution, push, lastRead]() mutable {
        // Normally when I'm consuming the time in a source function,
        // I expect a time source.
        // But because I know I'm on the Arduino platform,
        // we'll just use the millis clock.
        unsigned long now = millis();
        if (now - lastRead > sensor->millisToWaitForConversion(resolution)) {
          lastRead = now;
          float tempC = sensor->getTempC(address);
          if (tempC == DEVICE_DISCONNECTED_C) {
            push(std::nullopt);
          } else {
            push(au::celsius_pt(tempC));
          }
          sensor->requestTemperaturesByAddress(address);
        }
      };
    };
  }

  // A source function factory with a much nicer way of specifying a device address --
  // a 64-bit integer, which you can give as a hex value!
  source_fn<std::optional<au::QuantityPoint<au::Celsius, float>>> ds18b20(uint64_t address, DallasTemperature* sensor, Resolution resolution) {
    DeviceAddress address2;
    intToDeviceAddress(address, address2);
    return ds18b20(address2, sensor, resolution);
  }

}