#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/filter_map.hpp>
#include <Fallible.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Named callable for unwrap_optional's push handler
  template<typename T>
  struct unwrap_optional_push_handler {
    push_fn<T> push;

    RHEO_CALLABLE void operator()(std::optional<T> value) const {
      if (value.has_value()) {
        push(value.value());
      }
    }
  };

  // Named callable for unwrap_optional's source binder
  template<typename T>
  struct unwrap_optional_source_binder {
    source_fn<std::optional<T>> source;

    RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
      return source(unwrap_optional_push_handler<T>{push});
    }
  };

  template <typename T>
  RHEO_CALLABLE source_fn<T> unwrap_optional(source_fn<std::optional<T>> source) {
    return unwrap_optional_source_binder<T>{source};
  }

  template <typename T>
  pipe_fn<T, std::optional<T>> unwrap_optional() {
    return [](source_fn<std::optional<T>> source) {
      return unwrap_optional(source);
    };
  }

  // Named callable for unwrap_fallible's push handler
  template<typename T, typename TErr>
  struct unwrap_fallible_push_handler {
    push_fn<T> push;

    RHEO_CALLABLE void operator()(Fallible<T, TErr> value) const {
      if (value.is_ok()) {
        push(value);
      }
    }
  };

  // Named callable for unwrap_fallible's source binder
  template<typename T, typename TErr>
  struct unwrap_fallible_source_binder {
    source_fn<Fallible<T, TErr>> source;

    RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
      return source(unwrap_fallible_push_handler<T, TErr>{push});
    }
  };

  template <typename T, typename TErr>
  RHEO_CALLABLE source_fn<T> unwrap_fallible(source_fn<Fallible<T, TErr>> source) {
    return unwrap_fallible_source_binder<T, TErr>{source};
  }

  template <typename T, typename TErr>
  pipe_fn<T, Fallible<T, TErr>> unwrap_fallible() {
    return [](source_fn<Fallible<T, TErr>> source) {
      return unwrap_fallible(source);
    };
  }

  // Named callable for unwrap_endable's push handler
  template<typename T>
  struct unwrap_endable_push_handler {
    push_fn<T> push;

    RHEO_CALLABLE void operator()(Endable<T> value) const {
      if (value.has_value()) {
        push(value.value());
      }
    }
  };

  // Named callable for unwrap_endable's source binder
  template<typename T>
  struct unwrap_endable_source_binder {
    source_fn<Endable<T>> source;

    RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
      return source(unwrap_endable_push_handler<T>{push});
    }
  };

  template <typename T>
  RHEO_CALLABLE source_fn<T> unwrap_endable(source_fn<Endable<T>> source) {
    return unwrap_endable_source_binder<T>{source};
  }

  template <typename T>
  pipe_fn<T, Endable<T>> unwrap_endable() {
    return [](source_fn<Endable<T>> source) {
      return unwrap_endable(source);
    };
  }

}