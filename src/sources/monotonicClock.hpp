#ifndef RHEOSCAPE_MONOTONIC_CLOCK
#define RHEOSCAPE_MONOTONIC_CLOCK

#include <chrono>
#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

source_fn<mono_time_point> monotonicClock() {
  return [](push_fn<mono_time_point> push) {
    return [push]() {
      push(std::chrono::steady_clock::now());
    };
  };
}

#endif