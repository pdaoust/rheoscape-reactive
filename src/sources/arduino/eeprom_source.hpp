#pragma once

#include <types/core_types.hpp>
#include <types/Fallible.hpp>
#include <types/deserialization_error.hpp>
#include <Arduino.h>
#include <EEPROM.h>

namespace rheoscape::sources::arduino {

  // Read a typed value from the device's non-volatile storage,
  // using the device's platform EEPROM implementation.
  // Emits Fallible<T, deserialization_error> so consumers can detect
  // invalid data from uninitialised or corrupted EEPROM.

  namespace detail {

    template <typename T, typename PushFn>
    struct eeprom_source_pull_handler {
      int address;
      PushFn push;

      RHEOSCAPE_CALLABLE void operator()() const {
        using FallibleT = Fallible<T, deserialization_error>;
        T value;
        EEPROM.get(address, value);
        if (is_deserialized_valid(value)) {
          push(FallibleT(value));
        } else {
          push(FallibleT(deserialization_error{}));
        }
      }
    };

    template <typename T>
    struct eeprom_source_binder {
      using value_type = Fallible<T, deserialization_error>;
      int address;

      template <typename PushFn>
        requires concepts::Visitor<PushFn, value_type>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        return eeprom_source_pull_handler<T, PushFn>{address, std::move(push)};
      }
    };

  } // namespace detail

  template <typename T>
    requires concepts::Deserializable<T>
  auto eeprom_source(const int address) {
    return detail::eeprom_source_binder<T>{address};
  }

}
