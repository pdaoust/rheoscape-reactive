#ifndef RHEOSCAPE_BH1750
#define RHEOSCAPE_BH1750

#include <functional>
#include <memory>
#include <core_types.hpp>
#include <types/au_noio.hpp>
#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

source_fn<au::Quantity<au::LuminousIntensity, float>> bh1750(uint8_t address, TwoWire* i2c, BH1750::Mode mode = BH1750::Mode::CONTINUOUS_HIGH_RES_MODE) {
  return [address, i2c, mode](push_fn<au::Quantity<au::LuminousIntensity, float>> push, end_fn _) {
    auto sensor = std::make_shared<BH1750>();
    sensor->begin(mode, address, i2c);
    return [sensor, push, lastReadValue = std::optional<float>()]() mutable {
      if (sensor->measurementReady()) {
        lastReadValue.emplace(sensor->readLightLevel());
      }
      if (lastReadValue.has_value()) {
        push(au::lux(lastReadValue.value()));
      }
    };
  };
}

#endif