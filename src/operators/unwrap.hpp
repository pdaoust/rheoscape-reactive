#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/filterMap.hpp>
#include <Fallible.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Named callable for unwrapOptional's push handler
  template<typename T>
  struct unwrapOptional_push_handler {
    push_fn<T> push;

    RHEO_NOINLINE void operator()(std::optional<T> value) const {
      if (value.has_value()) {
        push(value.value());
      }
    }
  };

  // Named callable for unwrapOptional's source binder
  template<typename T>
  struct unwrapOptional_source_binder {
    source_fn<std::optional<T>> source;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return source(unwrapOptional_push_handler<T>{push});
    }
  };

  template <typename T>
  RHEO_INLINE source_fn<T> unwrapOptional(source_fn<std::optional<T>> source) {
    return unwrapOptional_source_binder<T>{source};
  }

  template <typename T>
  pipe_fn<T, std::optional<T>> unwrapOptional() {
    return [](source_fn<std::optional<T>> source) {
      return unwrapOptional(source);
    };
  }

  // Named callable for unwrapFallible's push handler
  template<typename T, typename TErr>
  struct unwrapFallible_push_handler {
    push_fn<T> push;

    RHEO_NOINLINE void operator()(Fallible<T, TErr> value) const {
      if (value.isOk()) {
        push(value);
      }
    }
  };

  // Named callable for unwrapFallible's source binder
  template<typename T, typename TErr>
  struct unwrapFallible_source_binder {
    source_fn<Fallible<T, TErr>> source;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return source(unwrapFallible_push_handler<T, TErr>{push});
    }
  };

  template <typename T, typename TErr>
  RHEO_INLINE source_fn<T> unwrapFallible(source_fn<Fallible<T, TErr>> source) {
    return unwrapFallible_source_binder<T, TErr>{source};
  }

  template <typename T, typename TErr>
  pipe_fn<T, Fallible<T, TErr>> unwrapFallible() {
    return [](source_fn<Fallible<T, TErr>> source) {
      return unwrapFallible(source);
    };
  }

  // Named callable for unwrapEndable's push handler
  template<typename T>
  struct unwrapEndable_push_handler {
    push_fn<T> push;

    RHEO_NOINLINE void operator()(Endable<T> value) const {
      if (value.hasValue()) {
        push(value.value());
      }
    }
  };

  // Named callable for unwrapEndable's source binder
  template<typename T>
  struct unwrapEndable_source_binder {
    source_fn<Endable<T>> source;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      return source(unwrapEndable_push_handler<T>{push});
    }
  };

  template <typename T>
  RHEO_INLINE source_fn<T> unwrapEndable(source_fn<Endable<T>> source) {
    return unwrapEndable_source_binder<T>{source};
  }

  template <typename T>
  pipe_fn<T, Endable<T>> unwrapEndable() {
    return [](source_fn<Endable<T>> source) {
      return unwrapEndable(source);
    };
  }

}