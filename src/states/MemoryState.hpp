#pragma once

#include <functional>
#include <types/core_types.hpp>

namespace rheoscape::states {

  // Forward declaration
  template <typename T>
  class MemoryState;

  namespace detail {

    // Named callable for MemoryState's pull handler
    template <typename T, typename PushFn>
    struct memory_state_pull_handler {
      MemoryState<T>* state;
      PushFn push;

      RHEOSCAPE_CALLABLE void operator()() const {
        auto value = state->try_get();
        if (value.has_value()) {
          push(value.value());
        }
        // Nothing set yet; don't push anything but don't error out either.
      }
    };

    // Named callable for MemoryState's source binder
    template <typename T>
    struct memory_state_source_binder {
      using value_type = T;
      MemoryState<T>* state;
      bool initial_push;

      template <typename PushFn>
        requires concepts::Visitor<PushFn, T>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        return state->add_sink(std::move(push), initial_push);
      }
    };

    template <typename T>
    struct memory_state_push_handler {
      MemoryState<T>* state;
      bool push_on_set;

      RHEOSCAPE_CALLABLE void operator()(T value) const {
        state->set(value, push_on_set);
      }
    };

    template <typename T>
    struct memory_state_sink_binder {
      MemoryState<T>* state;
      bool push_on_set;

      template <typename SourceFn>
        requires concepts::SourceOf<SourceFn, T>
      RHEOSCAPE_CALLABLE auto operator()(SourceFn source) const {
        return source(state->get_setter_push_fn(push_on_set));
      }
    };

  }

  template <typename T>
  class MemoryState {
    private:
      std::optional<T> _value;
      std::vector<push_fn<T>> _sinks;

    public:
      MemoryState() {}

      MemoryState(T initial, bool initial_push = true) {
        set(initial, initial_push);
      }

      void set(T value, bool push = true) {
        _value.emplace(value);

        if (push) {
          for (auto& sink : _sinks) {
            sink(value);
          }
        }
      }

      // Unsafe: Undefined behaviour if no value has been set.
      // Wrap `get()` in a conditional guarded by `has_value()`
      // or use `try_get()` for safe access.
      T get() {
        return _value.value();
      }

      std::optional<T> try_get() {
        return _value;
      }

      bool has_value() {
        return _value.has_value();
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
          if (_value.has_value()) {
            // Push the initial value.
            push(_value.value());
          }
        }

        // The pull function consumes errors
        // and turns them into meaningful action
        // (or meaningful inaction).
        return detail::memory_state_pull_handler<T, PushFn>{this, std::move(push)};
      }

      auto get_source_fn(bool initial_push = true) {
        return detail::memory_state_source_binder<T>{this, initial_push};
      }

      auto get_setter_push_fn(bool push_on_set = true) {
        return detail::memory_state_push_handler<T>{this, push_on_set};
      }

      auto get_setter_sink_fn(bool push_on_set = true) {
        return detail::memory_state_sink_binder<T>{this, push_on_set};
      }
  };

}
