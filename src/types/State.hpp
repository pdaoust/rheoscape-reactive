#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  class bad_state_unset_access : std::exception {
    public:
      const char* what() {
        return "Tried to access the state of a State object that was ended";
      }
  };

  template <typename T>
  class State {
    private:
      std::optional<T> _value;
      std::vector<push_fn<T>> _sinks;

    public:
      State(std::optional<T> initial = std::nullopt, bool initialPush = true) {
        if (initial.has_value()) {
          set(initial.value(), initialPush);
        }
      }

      void set(T value, bool push = true) {
        _value = value;

        if (push) {
          for (auto sink : _sinks) {
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
      pull_fn addSink(push_fn<T> push, bool initialPush = true) {
        _sinks.push_back(push);
        if (initialPush) {
          if (_value.has_value()) {
            // Push the initial value.
            push(_value.value());
          }
        }

        // The pull function consumes errors
        // and turns them into meaningful action
        // (or meaningful inaction).
        return [this, push]() {
          try {
            push(this->get());
          } catch (bad_state_unset_access e) {
            // Nothing set yet; don't push anything but don't error out either.
          }
        };
      }

      source_fn<T> sourceFn(bool initialPush = true) {
        return [this, initialPush](push_fn<T> push) { return this->addSink(push, initialPush); };
      }
  };

}