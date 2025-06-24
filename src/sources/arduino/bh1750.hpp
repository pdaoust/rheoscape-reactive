#pragma once

#include <functional>
#include <memory>
#include <core_types.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

namespace rheo::sources::arduino {

  source_fn<au::Quantity<au::Lux, float>> bh1750(uint8_t address, TwoWire* i2c, BH1750::Mode mode = BH1750::Mode::CONTINUOUS_HIGH_RES_MODE) {
    return [address, i2c, mode](push_fn<au::Quantity<au::Lux, float>> push) {
      auto sensor = std::make_shared<BH1750>();
      sensor->begin(mode, address, i2c);
      return [sensor, push]() {
        if (sensor->measurementReady()) {
          push(au::lux(sensor->readLightLevel()));
        }
      };
    };
  }

}