#pragma once

#include <cassert>
#include <typeinfo>
#include <EEPROM.h>
#include <CRCx.h>
#include <types/core_types.hpp>
#include <states/MemoryState.hpp>
#include <types/typed_pipe.hpp>

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
  // Given that you usually need to create ways for these EEPROM states to be updated,
  // and those ways usually take the form of pipes
  // (say, to buffer the values before saving so they don't wear the flash too quickly
  // and/or present feedback on how soon the states will be saved)
  // there's a convenience factory function that mirrors the EEPROM states into in-memory states
  // and lets you pass pipes that are inserted in between the memory states' source functions
  // and the EEPROM states' getter sink functions.
  // The type of each slot in the EEPROM will be deduced from the input/output type of the pipe function,
  // and the input/output must be identical.
  //
  // For instance:
  //
  // ```c++
  // auto persistent_configs = make_eeprom_states<0, Range<int>, int, bool, MyStruct>();
  // auto moisture_sensor_low_high_config = std::get<0>(persistent_configs);
  // auto watering_threshold_config = std::get<1>(persistent_configs);
  // auto is_running_config = std::get<2>(persistent_configs);
  // auto extra_config = std::get<3>(persistent_configs);
  //
  // // We're about to construct a pipeline to edit the config,
  // // but we never want to update an EEPROM directly as a result of user input.
  // // That would cause rapid flash wear.
  // // So instead we populate an in-memory state object
  // // and then buffer changes to that state so we only update flash when the value has settled.
  //
  // moisture_sensor_low_high_config.get_source_fn() // initial_push is true; that populates the in-mem state.
  //   | moisture_sensor_low_high_in_mem_state.get_setter_sink_function();
  // moisture_sensor_low_high_in_mem_state.get_source_fn()
  //   | settle(clock, arduino_millis_clock::duration(60000)) // Buffer the values for 1 minute.
  //   | moisture_sensor_low_high_config.get_setter_sink_fn(false); // Don't push on set; this'll cause an infinite loop.
  // // Not shown: pulling on these pipelines, updating the in-memory state.
  // ```

  namespace detail {

    template <typename T>
    class SizedHashedWrapper {
      uint16_t _hash;
      std::array<char, sizeof(T)> _data;

      public:
        SizedHashedWrapper() = default;

        SizedHashedWrapper(T original) {
          memcpy(_data.data(), &original, sizeof(T));
          _hash = CRCPP::CRC::Calculate(_data.data(), sizeof(T), CRCPP::CRC::CRC_16_MODBUS());
        }

        bool is_initialized() {
          return _hash != 0;
        }

        bool is_valid() {
          return is_initialized() && CRCPP::CRC::Calculate(_data.data(), sizeof(T), CRCPP::CRC::CRC_16_MODBUS()) == _hash;
        }

        T get_value() {
          if (!is_initialized()) {
            assert(false && "Value isn't initialized yet; no checksum");
          }

          if (!is_valid()) {
            assert(false && "Value isn't valid; checksum doesn't check out");
          }

          T value;
          memcpy(&value, _data.data(), sizeof(T));
          return value;
        }
    };

    // Forward declaration
    template <typename T, uint Offset>
    class EepromState;

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

    template <typename T, uint Offset>
    class EepromState {
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
          return eeprom_state_pull_handler<T, Offset, PushFn>{this, std::move(push)};
        }

        auto get_source_fn(bool initial_push = true) {
          return eeprom_state_source_binder<T, Offset>{this, initial_push};
        }

        auto get_setter_push_fn(bool push_on_set = true) {
          return eeprom_state_push_handler<T, Offset>{this, push_on_set};
        }

        auto get_setter_sink_fn(bool push_on_set = true) {
          return eeprom_state_sink_binder<T, Offset>{this, push_on_set};
        }
    };

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

      static type make() {
        return std::tuple_cat(
          std::forward_as_tuple(EepromState<T, Offset>::get_instance()),
          EepromStateTupleBuilder<Offset + sizeof(T) + 2, Rest...>::make()
        );
      }
    };

    // Helper to get the tail of a tuple (all elements except the first).
    template <typename Tuple, std::size_t... Is>
    auto tuple_tail_impl(Tuple&& t, std::index_sequence<Is...>) {
      return std::make_tuple(std::get<Is + 1>(std::forward<Tuple>(t))...);
    }

    template <typename Tuple>
    auto tuple_tail(Tuple&& t) {
      return tuple_tail_impl(
        std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>> - 1>{}
      );
    }

    template<std::size_t Offset, typename... PipeFns>
    struct MemoryMirroredEepromStateTupleBuilder;

    template<std::size_t Offset>
    struct MemoryMirroredEepromStateTupleBuilder<Offset> {
      std::tuple<> pipes;

      using type = std::tuple<>;
      type make() { return {}; }
    };

    template<std::size_t Offset, typename PipeFn, typename... RestPipeFns>
    struct MemoryMirroredEepromStateTupleBuilder<Offset, PipeFn, RestPipeFns...> {
      std::tuple<PipeFn, RestPipeFns...> pipes;

      using T = pipe_input_t<PipeFn>;

      using type = decltype(std::tuple_cat(
        std::declval<std::tuple<MemoryState<T>>>(),
        std::declval<typename MemoryMirroredEepromStateTupleBuilder<Offset + sizeof(T) + 2, RestPipeFns...>::type>()
      ));

      type make() {
        type result = std::tuple_cat(
          std::make_tuple(MemoryState<T>()),
          MemoryMirroredEepromStateTupleBuilder<Offset + sizeof(T) + 2, RestPipeFns...>{tuple_tail(pipes)}.make()
        );

        MemoryState<T>& memory_state = std::get<0>(result);
        auto& eeprom_state = EepromState<T, Offset>::get_instance();
        // Push_initial defaults to true,
        // so the memory state gets populated on bind.
        eeprom_state.get_source_fn()
          // Push_on_set defaults to true,
          // keeping the pipe alive w/o a pull function.
          | memory_state.get_setter_sink_fn();
        memory_state.get_source_fn()
          | std::get<0>(pipes)
          // Avoid infinite loops from eeprom to memory and back!
          | eeprom_state.get_setter_sink_fn(false);
        return result;
      }
    };
  }

  template <uint Offset, typename... Ts>
  auto make_eeprom_states() {
    return detail::EepromStateTupleBuilder<Offset, Ts...>::make();
  }

  // Creates a tuple of MemoryState objects, each mirrored to a corresponding EEPROM slot.
  // For each slot, two pipelines are set up:
  //   1. EEPROM -> MemoryState (initial populate on bind)
  //   2. MemoryState -> pipe -> EEPROM (buffered write-back)
  //
  // Each pipe must satisfy the Pipe concept
  // and have matching input and output types
  // (since the value goes from MemoryState<T> through the pipe back to EepromState<T>).
  // Use typed_pipe<T>() to annotate an untyped pipe factory with its value type.
  template <uint Offset, typename... PipeFns>
    requires (concepts::Pipe<std::decay_t<PipeFns>> && ...)
      && (std::same_as<pipe_input_t<PipeFns>, pipe_output_t<PipeFns>> && ...)
  auto make_memory_mirrored_eeprom_pipes(PipeFns... pipes) {
    return detail::MemoryMirroredEepromStateTupleBuilder<Offset, std::decay_t<PipeFns>...>{
      std::make_tuple(std::move(pipes)...)
    }.make();
  }

}