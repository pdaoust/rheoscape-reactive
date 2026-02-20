#pragma once

#include <types/core_types.hpp>
#include <Arduino.h>

namespace rheoscape::sources::arduino {

  namespace detail {

    template <typename PushFn>
    struct digital_pin_pull_handler {
      int pin;
      PushFn push;

      RHEOSCAPE_CALLABLE void operator()() const {
        push(static_cast<bool>(digitalRead(pin)));
      }
    };

    struct digital_pin_source_binder {
      using value_type = bool;
      int pin;

      template <typename PushFn>
        requires concepts::Visitor<PushFn, bool>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        return digital_pin_pull_handler<PushFn>{pin, std::move(push)};
      }
    };

  } // namespace detail

  auto digital_pin_source(int pin, uint8_t pin_mode_flag) {
    pinMode(pin, INPUT | pin_mode_flag);
    return detail::digital_pin_source_binder{pin};
  }

  namespace detail {

    // Two buffers storing pin changes; up to 32 per pin before old values are dropped.
    // The value for each pin is a sequence of changes (0 is LOW; 1 is HIGH) plus a bit mask.
    static volatile uint32_t _pin_changes[NUM_DIGITAL_PINS] = {0};
    static volatile uint32_t _pin_changes_bitmasks[NUM_DIGITAL_PINS] = {0};

    // Interrupt handler.
    template <int... Pins>
    void handle_pin_change() {
      for (int pin : {Pins...}) {
        uint32_t changes = _pin_changes[pin];
        uint32_t bitmask = _pin_changes_bitmasks[pin];
        changes <<= 1;
        changes |= digitalRead(pin);
        _pin_changes[pin] = changes;
        _pin_changes_bitmasks[pin] = bitmask;
      }
    }

    // Apparently you can't expand a variadic template parameter pack in a macro call,
    // so this function just wraps it.
    auto digitalPinToInterrupt_nonMacro(int pin) {
      return digitalPinToInterrupt(pin);
    }

    template <typename PushFn, int... Pins>
    struct digital_pin_interrupt_push_handler {
      private:
        template <std::size_t... Is>
        auto make_values(uint32_t* pin_changes, int pos, std::index_sequence<Is...>) const {
          return std::make_tuple(((pin_changes[Is] >> pos) & 1)...);
        }

      public:
        PushFn push;

        RHEOSCAPE_CALLABLE void operator()() const {
          // Atomic snapshot
          noInterrupts();
          uint32_t pin_changes[] = {_pin_changes[Pins]...};
          uint32_t bitmasks[] = {_pin_changes_bitmasks[Pins]...};
          uint32_t bitmask = bitmasks[0];
          for (int pin : {Pins...}) {
            _pin_changes[pin] = 0;
            _pin_changes_bitmasks[pin] = 0;
          }
          interrupts();
          // End atomic snapshot

          while (bitmask) {
            // Lots of fancy bit math here.
            // First, find the position of the leftmost (oldest) bit.
            int pos = 31 - __builtin_clz(bitmask);
            // Then construct a tuple of all the bits.
            auto values = make_values(pin_changes, pos, std::make_index_sequence<sizeof...(Pins)>{});
            // Clear that bit.
            bitmask &= ~(1u << pos);
            // Now, the moment we've been waiting for.
            push(values);
          }
        }
    };

    template <int... Pins>
    struct digital_pin_interrupt_source_binder {
      using value_type = std::tuple<decltype(Pins, bool{})...>;

      template <typename PushFn>
      // TODO: can I add a concept constraint here or is SFINAE good enough?
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        return digital_pin_interrupt_push_handler<PushFn, Pins...>{std::move(push)};
      }
    };

  }

  template <int... Pins>
  auto digital_pin_interrupt_source(uint8_t pin_mode_flag) {
    (pinMode(Pins, INPUT | pin_mode_flag), ...);
    auto handler = detail::handle_pin_change<Pins...>;
    (attachInterrupt(detail::digitalPinToInterrupt_nonMacro(Pins), handler, CHANGE), ...);
    return detail::digital_pin_interrupt_source_binder<Pins...>{};
  }

}
