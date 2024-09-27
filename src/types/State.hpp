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
      State(std::optional<T> initial = std::nullopt)
      : _isEnded(false)
      {
        if (initial.has_value()) {
          set(initial.value());
        }
      }

      void set(T value) {
        if (_isEnded) {
          throw bad_state_ended_access();
        }

        _value = value;
        for (auto subscriber : _sinks) {
          std::get<0>(subscriber)(value);
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

      void end() {
        if (_isEnded) {
          return;
        }

        _isEnded = true;
        for (auto sink : _sinks) {
          std::get<1>(sink)();
        }
      }

      bool isEnded() {
        return _isEnded;
      }

      // This can be used as-is as a source function.
      pull_fn addSink(push_fn<T> push, end_fn end) {
        _sinks.push_back(std::tuple(push, end));
        if (_value.has_value()) {
          // Push the initial value.
          push(_value.value());
        } else if (_isEnded) {
          end();
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

      source_fn<T> sourceFn() {
        return [this](push_fn<T> push, end_fn end) { return this->addSink(push, end); };
      }
  };

}