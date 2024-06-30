#ifndef RHEOSCAPE_STATE
#define RHEOSCAPE_STATE

#include <functional>
#include <core_types.hpp>

template <typename T>
class State {
  private:
    T _value;
  
  public:
    State(T initial)
    : _value(initial)
    { }

    void set(T value) {
      _value = value;
    }

    T get() {
      return _value;
    }
};

template <typename T>
source_fn<T> state(State<T>& state) {
  return [state](push_fn<T> push) {
    return [push, state](){ push(state->get()); };
  };
}

#endif