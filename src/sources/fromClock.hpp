#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  template <typename TClock>
  source_fn<typename TClock::time_point> fromClock() {
    return [](push_fn<typename TClock::time_point> push, end_fn _) {
      return [push]() {
        push(std::move(TClock::now()));
      };
    };
  }

}