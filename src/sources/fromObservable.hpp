#pragma once

#include <array>
#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  template <typename T>
  source_fn<T> fromObservable(std::function<void(std::function<void(T)>)> subscribeFn) {
    return [subscribeFn](push_fn<T> push, end_fn end) {
      subscribeFn([push](T value) { push(value); });
      // It doesn't mean anything to pull from an observable.
      return [](){};
    };
  }

}