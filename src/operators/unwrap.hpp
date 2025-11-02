#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/filterMap.hpp>
#include <Fallible.hpp>
#include <Endable.hpp>


namespace rheo::operators {
  
  template <typename T>
  source_fn<T> unwrapOptional(source_fn<std::optional<T>> source) {
    return [source](push_fn<T> push) {
      return source([push](std::optional<T> value) {
        if (value.has_value()) {
          push(value.value());
        }
      });
    };
  }

  template <typename T>
  pipe_fn<T, std::optional<T>> unwrapOptional() {
    return [](source_fn<T> source) {
      return unwrapOptional(source);
    };
  }

  template <typename T, typename TErr>
  source_fn<T> unwrapFallible(source_fn<Fallible<T, TErr>> source) {
    return [source](push_fn<T> push) {
      return source([push](Fallible<T, TErr> value) {
        if (value.isOk()) {
          push(value);
        }
      });
    };
  }

  template <typename T, typename TErr>
  pipe_fn<T, Fallible<T, TErr>> unwrapFallible() {
    return [](source_fn<Fallible<T, TErr>> source) {
      return unwrapFallible(source);
    };
  }


  template <typename T>
  source_fn<T> unwrapEndable(source_fn<Endable<T>> source) {
    return [source](push_fn<T> push) {
      return source([push](Endable<T> value) {
        if (value.hasValue()) {
          push(value.value());
        }
      });
    };
  }

  template <typename T>
  pipe_fn<T, Endable<T>> unwrapEndable() {
    return [](source_fn<Endable<T>> source) {
      return unwrapEndable(source);
    };
  }

}