#pragma once
#if __has_include(<EEPROM.h>)

#include <types/core_types.hpp>
#include <Arduino.h>
#include <EEPROM.h>

namespace rheoscape::sinks::arduino {

  // Write a typed value to the device's non-volatile storage,
  // using the device's platform EEPROM implementation.
  // WARNING: the standard rules about writing to NVRAM apply;
  // if your EEPROM is actually flash behind the scenes,
  // you WILL wear it out if you write to it often.
  // If you're handling a quick-changing and noisy source like live user input,
  // put a pipeline like `dedupe` + `settle` to avoid writing identical values
  // and introduce a delay before writing.

  namespace detail {

    template <typename T>
    struct eeprom_push_handler {
      int address;

      RHEOSCAPE_CALLABLE void operator()(T value) const {
        EEPROM.put(address, value);
      }
    };

    template <typename T>
    struct eeprom_sink_binder {
      int address;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, T>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        return source(eeprom_push_handler<T>{address});
      }
    };

  } // namespace detail

  template <typename T>
  auto eeprom_sink(const int address) {
    return detail::eeprom_sink_binder<T>{address};
  }

}
#endif // __has_include(<EEPROM.h>)
