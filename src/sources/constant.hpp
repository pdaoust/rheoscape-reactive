#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  template <typename T>
  source_fn<T> constant(T&& value) {
    return [value = std::forward<T>(value)](push_fn<T>&& push) {
      return [push = std::forward<push_fn<T>>(push), value](){ push(value); };
    };
  }

}