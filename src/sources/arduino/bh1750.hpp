#pragma once

#include <memory>
#include <types/core_types.hpp>
#include <types/au_all_units_noio.hpp>
#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

namespace rheoscape::sources::arduino {

  namespace detail {

    template <typename PushFn>
    struct bh1750_pull_handler {
      std::shared_ptr<BH1750> sensor;
      PushFn push;

      RHEOSCAPE_CALLABLE void operator()() const {
        if (sensor->measurementReady()) {
          push(au::lux(sensor->readLightLevel()));
        }
      }
    };

    struct bh1750_source_binder {
      using value_type = au::Quantity<au::Lux, float>;
      std::shared_ptr<BH1750> sensor;

      template <typename PushFn>
        requires concepts::Visitor<PushFn, value_type>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        return bh1750_pull_handler<PushFn>{sensor, std::move(push)};
      }
    };

  } // namespace detail

  auto bh1750(uint8_t address, TwoWire* i2c, BH1750::Mode mode = BH1750::Mode::CONTINUOUS_HIGH_RES_MODE) {
    auto sensor = std::make_shared<BH1750>();
    sensor->begin(mode, address, i2c);
    return detail::bh1750_source_binder{sensor};
  }

}
