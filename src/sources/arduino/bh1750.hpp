#ifndef RHEOSCAPE_BH175-
#define RHEOSCAPE_BH175-

#include <functional>
#include <core_types.hpp>
#include <types/au_noio.hpp>
#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

source_fn<au::Quantity<au::LuminousIntensity, float>> bh1750(uint8_t address, TwoWire& i2c) {
  return [address, i2c](push_fn<float> push, end_fn _) {
    auto sensor = std::make_shared(BH1750(address));
    sensor->begin(BH1750::Mode::CONTINUOUS_HIGH_RES_MODE, address, i2c);
    return [sensor, push, lastReadValue = std::optional<float>()]() {
      if (sensor->measurementReady()) {
        lastReadValue = lightMeter.readLightLevel();
      }
      if (lastReadValue->has_value()) {
        push(au::lux(lastreadValue->value()));
      }
    };
  };
}

#endif