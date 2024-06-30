#ifndef RHEOSCAPE_CLOCK_SOURCE
#define RHEOSCAPE_CLOCK_SOURCE

#include <functional>
#include <core_types.hpp>
#include <types/TSValue.hpp>
#include <Arduino.h>

template <typename TClock>
source_fn<typename TClock::time_point> clockSource() {
  return [](push_fn<typename TClock::time_point> push, end_fn _) {
    return [push]() {
      push(std::move(TClock::now()));
    };
  };
}

#endif