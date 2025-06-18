#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  class bad_state_ended_access : std::exception {
    public:
      const char* what() {
        return "Tried to get or set the state of a State object that was ended";
      }
  };

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
      bool _isEnded;
      std::vector<std::tuple<push_fn<T>, end_fn>> _sinks;

    public:
      State(std::optional<T> initial = std::nullopt, bool initialPush = true)
      : _isEnded(false)
      {
        if (initial.has_value()) {
          set(initial.value(), initialPush);
        }
      }

      void set(T value, bool push = true) {
        if (_isEnded) {
          throw bad_state_ended_access();
        }

        _value = value;

        if (push) {
          for (auto subscriber : _sinks) {
            std::get<0>(subscriber)(value);
          }
        }
      }

      T get() {
        if (_isEnded) {
          throw bad_state_ended_access();
        }
        if (!_value.has_value()) {
          throw bad_state_unset_access();
        }
        return _value.value();
      }

      void end(bool push = true) {
        if (_isEnded) {
          return;
        }

        _isEnded = true;

        if (push) {
          for (auto sink : _sinks) {
            std::get<1>(sink)();
          }
        }
      }

      bool isEnded() {
        return _isEnded;
      }

      // This can be used as-is as a source function.
      pull_fn addSink(push_fn<T> push, end_fn end, bool initialPush = true) {
        _sinks.push_back(std::tuple(push, end));
        if (initialPush) {
          if (_value.has_value()) {
            // Push the initial value.
            push(_value.value());
          } else if (_isEnded) {
            end();
          }
        }

        // The pull function consumes errors
        // and turns them into meaningful action
        // (or meaningful inaction).
        return [this, push, end]() {
          try {
            push(this->get());
          } catch (bad_state_ended_access e) {
            end();
          } catch (bad_state_unset_access e) {
            // Nothing set yet; don't push anything but don't error out either.
          }
        };
      }

      source_fn<T> sourceFn(bool initialPush = true) {
        return [this, initialPush](push_fn<T> push, end_fn end) { return this->addSink(push, end, initialPush); };
      }
  };

}