#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Named callable for take's push handler
  template<typename T>
  struct take_push_handler {
    size_t count;
    push_fn<Endable<T>> push;
    mutable size_t i = 0;

    RHEO_CALLABLE void operator()(T&& value) const {
      if (i < count) {
        push(Endable<T>(std::forward<T>(value), i == count - 1));
        i++;
      } else {
        push(Endable<T>());
      }
    }
  };

  // Named callable for take's source binder
  template<typename T>
  struct take_source_binder {
    source_fn<T> source;
    size_t count;

    RHEO_CALLABLE pull_fn operator()(push_fn<Endable<T>> push) const {
      return source(take_push_handler<T>{count, push});
    }
  };

  // Re-emit a number of values from the source function,
  // then end the source.
  template <typename T>
  RHEO_CALLABLE source_fn<Endable<T>> take(source_fn<T> source, size_t count) {
    return take_source_binder<T>{source, count};
  }

  template <typename T>
  pipe_fn<Endable<T>, T> take(size_t count) {
    return [count](source_fn<T> source) {
      return take(source, count);
    };
  }

}