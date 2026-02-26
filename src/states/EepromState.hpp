#pragma once

#include <cassert>
#include <memory>
#include <typeinfo>
#include <EEPROM.h>
#include <CRCx.h>
#include <types/core_types.hpp>
#include <states/MemoryState.hpp>
#include <util/pipes.hpp>

using namespace rheoscape;
using namespace rheoscape::util;

namespace rheoscape::states {

  // EEPROM sources and sinks -- actually, they're state objects that provide source and sink functions
  // and getter and setter functions.
  // The internals handle checksumming to make sure that the type you expect is actually sane data.
  // Internally, they use the Arduino EEPROM library, which means:
  //
  // * putting a value only updates the bytes that have changed (along with the two checksum bytes at the beginning)
  // * you should follow your hardware's recommendations and warnings about writing too often
  //   -- this tool doesn't do any sort of wear levelling.
  //
  // You don't create individual state objects for each slot in the EEPROM.
  // Instead, you call the `make_eeprom_states` factory function
  // with the desired starting memory offset and the types for each slot.
  // In return, you get a tuple of `EepromState` objects that correspond to the types you passed.
  //
  // When you're using NVRAM memory, it's usually because you're working with user-configurable settings.
  // But you don't want to be writing to NVRAM with every twiddle of a rotary encoder.
  // So it's a good idea to construct a pipeline that loads the value from NVRAM into a `MemoryState`,
  // create a loop to process and update it in there,
  // and then buffer changes using an appropriate pipeline.
  // This is such a common pattern that Rheoscape provides a factory function, `make_memory_mirrored_eeprom_pipes`,
  // which constructs an `EepromState` object and `MemoryState` object for each type,
  // then pipes their inputs and outputs to each other,
  // inserting the passed pipe between the `MemoryState` object's output and the `EepromState`'s input.
  // The type of each EEPROM value is deduced from the input and output types of the pipe,
  // which must be identical.
  // What you get back are not `EepromState` objects but `MemoryState` objects,
  // which should be more foolproof to work with.
  //
  // Example:
  //
  // ```c++
  // auto clock = from_clock<arduino_millis_clock>();
  // // Create a 1-minute buffer pipe that waits for a value to settle before emitting.
  // // Using `settle` allows us to both avoid writing intermediate values while the user is making up their mind,
  // // and avoid writing a value if they revert it back to what it was before.
  // auto buffer_writes_pipe = settle(clock, constant(arduino_millis_clock::duration(60000)));
  //
  // auto [
  //   moisture_sensor_low_high,
  //   watering_threshold,
  //   is_running,
  //   extra_config
  // ] = make_memory_mirrored_eeprom_pipes(
  //   // Tip: use no_option<T> to specify the value type without a default value.
  //   no_option<Range<int>>, buffer_writes_pipe,
  //   no_option<int>, buffer_writes_pipe,
  //   no_option<bool>, buffer_writes_pipe,
  //   no_option<MyConfigStruct>, buffer_writes_pipe
  // );
  // ```
  //
  // Here are two other suggestions for pipes.
  // This one allows you to give the user feedback about how soon their new value will be written:
  //
  // ```c++
  // auto timer_feedback_buffer_writes_pipe = compose_pipes(
  //   stopwatch_changes(clock),
  //   tee(some_ui_pipe_that_displays_seconds_until_ts_reaches_60000),
  //   filter([](auto value, auto ts) { return ts >= arduino_millis_clock::duration(60000); }),
  //   map([](auto value, auto ts) { return value; }),
  //   dedupe() // We only want the first value after it changes.
  // );
  // ```
  //
  // This one waits for the user to press a 'save' button:
  //
  // ```c++
  // auto manual_save_pipe = sample_every(save_button_events);
  // ```

  // Forward declaration
  template <typename T, uint Offset>
  class EepromState;

  namespace detail {

    template <typename T>
    class SizedHashedWrapper {
      uint16_t _hash;
      std::array<char, sizeof(T)> _data;

      public:
        SizedHashedWrapper() {
          static_assert(std::is_trivially_copyable_v<T>);
        }

        SizedHashedWrapper(T original) : SizedHashedWrapper() {
          memcpy(_data.data(), &original, sizeof(T));
          _hash = crcx::crc16(reinterpret_cast<const uint8_t*>(_data.data()), sizeof(T));
        }

        bool is_initialized() {
          return _hash != 0;
        }

        bool is_valid() {
          return is_initialized() && crcx::crc16(reinterpret_cast<const uint8_t*>(_data.data()), sizeof(T)) == _hash;
        }

        T get_value() {
          if (!is_initialized()) {
            assert(false && "Value isn't initialized yet; no checksum");
          }

          if (!is_valid()) {
            assert(false && "Value isn't valid; checksum doesn't check out");
          }

          alignas(T) unsigned char buffer[sizeof(T)];
          T* ptr = reinterpret_cast<T*>(buffer);
          memcpy(buffer, _data.data(), sizeof(T));
          return *ptr;
        }
    };

    // Named callable for EepromState's pull handler
    template <typename T, uint Offset, typename PushFn>
    struct eeprom_state_pull_handler {
      EepromState<T, Offset>* state;
      PushFn push;

      RHEOSCAPE_CALLABLE void operator()() const {
        auto value = state->try_get();
        if (value.has_value()) {
          push(value.value());
        }
        // Nothing set yet; don't push anything but don't error out either.
      }
    };

    // Named callable for EepromState's source binder
    template <typename T, uint Offset>
    struct eeprom_state_source_binder {
      using value_type = T;
      EepromState<T, Offset>* state;
      bool initial_push;

      template <typename PushFn>
        requires concepts::Visitor<PushFn, T>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        return state->add_sink(std::move(push), initial_push);
      }
    };

    template <typename T, uint Offset>
    struct eeprom_state_push_handler {
      EepromState<T, Offset>* state;
      bool push_on_set;

      RHEOSCAPE_CALLABLE void operator()(T value) const {
        state->set(value, push_on_set);
      }
    };

    template <typename T, uint Offset>
    struct eeprom_state_sink_binder {
      EepromState<T, Offset>* state;
      bool push_on_set;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, T>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        return source(state->get_setter_push_fn(push_on_set));
      }
    };

  }

  template <typename T, uint Offset>
  class EepromState {
    EepromState(T initial) {
      if (!has_value()) {
        set(initial);
      }
    }

    EepromState() = default;
    EepromState(const EepromState<T, Offset>&) = delete;
    EepromState<T, Offset>& operator=(const EepromState<T, Offset>&) = delete;

    std::vector<push_fn<T>> _sinks;

    detail::SizedHashedWrapper<T> _get_raw() {
        detail::SizedHashedWrapper<T> data;
        EEPROM.get(Offset, data);
        return data;
    }

    public:
      // Singleton 'constructor'.
      static EepromState<T, Offset>& get_instance() {
        static EepromState<T, Offset> instance;
        return instance;
      }

      static EepromState<T, Offset>& get_instance(T initial) {
        static EepromState<T, Offset> instance(initial);
        return instance;
      }

      void set(T value, bool push = true) {
        detail::SizedHashedWrapper<T> data(value);
        EEPROM.put(Offset, data);

        if (push) {
          for (auto& sink : _sinks) {
            sink(value);
          }
        }
      }

      T get() {
        detail::SizedHashedWrapper<T> data = _get_raw();
        if (!data.is_valid()) {
          assert(false && "Data at EEPROM address isn't valid; checksum doesn't check out");
        }
        return data.get_value();
      }

      std::optional<T> try_get() {
        detail::SizedHashedWrapper<T> data = _get_raw();
        if (!data.is_valid()) {
          return std::nullopt;
        }
        return data.get_value();
      }

      bool has_value() {
        return _get_raw().is_valid();
      }

      // This can be used as-is as a source function.
      // The PushFn is type-erased into push_fn<T> for storage
      // in the internal sinks vector, but the returned pull handler
      // retains the concrete PushFn type.
      template <typename PushFn>
        requires concepts::Visitor<PushFn, T>
      auto add_sink(PushFn push, bool initial_push = true) {
        _sinks.push_back(push_fn<T>(push));
        if (initial_push) {
          detail::SizedHashedWrapper<T> data = _get_raw();
          if (data.is_valid()) {
            // Push the initial value.
            push(data.get_value());
          }
        }

        // The pull function consumes errors
        // and turns them into meaningful action
        // (or meaningful inaction).
        return detail::eeprom_state_pull_handler<T, Offset, PushFn>{this, std::move(push)};
      }

      auto get_source_fn(bool initial_push = true) {
        return detail::eeprom_state_source_binder<T, Offset>{this, initial_push};
      }

      auto get_setter_push_fn(bool push_on_set = true) {
        return detail::eeprom_state_push_handler<T, Offset>{this, push_on_set};
      }

      auto get_setter_sink_fn(bool push_on_set = true) {
        return detail::eeprom_state_sink_binder<T, Offset>{this, push_on_set};
      }
  };

  namespace detail {
    template<std::size_t Offset, typename... Ts>
    struct EepromStateTupleBuilder;

    template<std::size_t Offset>
    struct EepromStateTupleBuilder<Offset> {
      using type = std::tuple<>;
      static type make() { return {}; }
    };

    template<std::size_t Offset, typename T, typename... Rest>
    struct EepromStateTupleBuilder<Offset, T, Rest...> {
      using type = decltype(std::tuple_cat(
        std::declval<std::tuple<EepromState<T, Offset>&>>(),
        std::declval<typename EepromStateTupleBuilder<Offset + sizeof(T) + 2, Rest...>::type>()
      ));

      static type make(std::optional<T> initial, Rest... rest) {
      }
    };

    template <uint Offset, typename T, typename PipeFn>
    auto make_memory_mirrored_eeprom_pipe(MemoryState<T>& memory_state, std::optional<T> initial, PipeFn pipe) {
      auto& eeprom_state = initial.has_value()
        ? EepromState<T, Offset>::get_instance(initial.value())
        : EepromState<T, Offset>::get_instance();

      // push_initial defaults to true,
      // so the memory state gets populated on bind.
      eeprom_state.get_source_fn()
        // Push_on_set defaults to true,
        // keeping the pipe alive w/o a pull function.
        | memory_state.get_setter_sink_fn();
      memory_state.get_source_fn()
        | pipe
        // Avoid infinite loops from eeprom to memory and back!
        | eeprom_state.get_setter_sink_fn(false);
    }
  }

  template <uint Offset>
  std::tuple<> make_eeprom_states() {
    return std::tuple<>{};
  }

  template <uint Offset, typename T, typename... Rest>
  auto make_eeprom_states() {
    return std::tuple_cat(
      std::forward_as_tuple(EepromState<T, Offset>::get_instance()),
      make_eeprom_states<Offset + sizeof(detail::SizedHashedWrapper<T>), Rest...>()
    );
  }

  template <uint Offset, typename T, typename... Rest>
  auto make_eeprom_states(std::optional<T> initial, Rest... rest) {
    return std::tuple_cat(
      std::forward_as_tuple(initial.has_value()
        ? EepromState<T, Offset>::get_instance(initial.value())
        : EepromState<T, Offset>::get_instance()
      ),
      make_eeprom_states<Offset + sizeof(T) + 2>(rest...)
    );
  }

  // Creates a tuple of MemoryState objects, each mirrored to a corresponding EEPROM slot.
  // For each slot, two pipelines are set up:
  //   1. EEPROM -> MemoryState (initial populate on bind)
  //   2. MemoryState -> pipe -> EEPROM (buffered write-back)
  //
  // Each initial value must be a std::optional,
  // and each corresponding pipe must be able to be bound to the initial value's type
  // and produce the exact same output type as the initial value's type.
  //
  // Usage:
  //
  // ```c++
  // auto [
  //   threshold_state,
  //   extra_config_state,
  //   is_running_state
  // ] = make_memory_mirrored_eeprom_pipes(
  //   option(3.14f), manual_save_pipe,
  //   option(MyStruct{ "hello", 16 }), manual_save_pipe,
  //   option(false), buffer_writes_pipe
  // );

  // End case.
  // Implementation uses shared_ptr<MemoryState<T>> internally
  // to ensure MemoryState addresses remain stable after pipeline binding.
  // The binders and handlers in MemoryState/EepromState capture `this` pointers,
  // which would dangle if the MemoryState were moved after binding.
  //
  // The returned tuple contains MemoryState<T>& references
  // (same as make_eeprom_states returns EepromState references),
  // with ownership managed by a shared_ptr stored inside the pipeline closures.

  namespace detail {
    template <uint Offset>
    std::tuple<> make_memory_mirrored_eeprom_pipes_impl() {
      return std::tuple<>{};
    }

    template <uint Offset, typename T, typename PipeFn, typename... Rest>
    auto make_memory_mirrored_eeprom_pipes_impl(std::optional<T> initial, PipeFn pipe, Rest... rest) {
      auto mem_ptr = std::make_shared<MemoryState<T>>();
      make_memory_mirrored_eeprom_pipe<Offset>(*mem_ptr, initial, pipe);

      // Prevent the MemoryState from being destroyed
      // by capturing the shared_ptr in the EepromState's sink.
      // The EepromState is a singleton so this effectively gives static lifetime.
      auto& eeprom_state = initial.has_value()
        ? EepromState<T, Offset>::get_instance(initial.value())
        : EepromState<T, Offset>::get_instance();
      eeprom_state.add_sink(
        [mem_ptr](T) {
          // No-op sink; exists solely to prevent the shared_ptr
          // (and thus the MemoryState) from being destroyed.
        },
        false
      );

      return std::tuple_cat(
        std::forward_as_tuple(*mem_ptr),
        make_memory_mirrored_eeprom_pipes_impl<Offset + sizeof(SizedHashedWrapper<T>)>(rest...)
      );
    }
  }

  template <uint Offset, typename T, typename PipeFn, typename... Rest>
  auto make_memory_mirrored_eeprom_pipes(std::optional<T> initial, PipeFn pipe, Rest... rest) {
    return detail::make_memory_mirrored_eeprom_pipes_impl<Offset>(initial, pipe, rest...);
  }

}