#ifndef RHEOSCAPE_STATE
#define RHEOSCAPE_STATE

#include <functional>
#include <core_types.hpp>

class bad_state_ended_access {
  public:
    const char* what() {
      return "Tried to access the state of a State object that was ended";
    }
};

template <typename T>
class State {
  private:
    T _value;
    bool _ended;
  
  public:
    State(T initial)
    : _value(initial)
    { }

    void set(T value) {
      _value = value;
    }

    T get() {
      if (_ended) {
        throw new bad_state_ended_access;
      }
      return _value;
    }

    void end() {
      _ended = true;
    }

    bool isEnded() {
      return _ended;
    }
};

template <typename T>
source_fn<T> state(State<T>& state) {
  return [state](push_fn<T> push, end_fn end) {
    return [state, push, end, alreadyEnded = false]() mutable {
      if (alreadyEnded) {
        return;
      }
      if (state->isEnded()) {
        alreadyEnded = true;
        end();
      } else {
        push(state->get());
      }
    };
  };
}

#endif