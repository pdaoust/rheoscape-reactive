#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  class bad_state_unset_access : std::exception {
    public:
      const char* what() const noexcept {
        return "Tried to access the state of a State object that was ended";
      }
  };

  // Forward declaration
  template <typename T>
  class State;

  // Named callable for State's pull handler
  template <typename T>
  struct state_pull_handler {
    State<T>* state;
    push_fn<T> push;

    RHEO_NOINLINE void operator()() const {
      try {
        push(state->get());
      } catch (const bad_state_unset_access&) {
        // Nothing set yet; don't push anything but don't error out either.
      }
    }
  };

  // Named callable for State's source binder
  template <typename T>
  struct state_source_binder {
    State<T>* state;
    bool initial_push;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return state->add_sink(std::move(push), initial_push);
    }
  };

  template <typename T>
  class State {
    private:
      std::optional<T> _value;
      std::vector<push_fn<T>> _sinks;

    public:
      State() {}

      State(T initial, bool initial_push = true) {
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

      T get() {
        if (!_value.has_value()) {
          throw bad_state_unset_access();
        }
        return _value.value();
      }

      // This can be used as-is as a source function.
      pull_fn add_sink(push_fn<T> push, bool initial_push = true) {
        _sinks.push_back(push);
        if (initial_push) {
          if (_value.has_value()) {
            // Push the initial value.
            push(_value.value());
          }
        }

        // The pull function consumes errors
        // and turns them into meaningful action
        // (or meaningful inaction).
        return state_pull_handler<T>{this, std::move(push)};
      }

      source_fn<T> get_source_fn(bool initial_push = true) {
        return state_source_binder<T>{this, initial_push};
      }
  };

}