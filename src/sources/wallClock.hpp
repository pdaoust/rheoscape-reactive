#ifndef RHEOSCAPE_WALL_CLOCK
#define RHEOSCAPE_WALL_CLOCK

#include <chrono>
#include <functional>
#include <core_types.hpp>
#include <Arduino.h>

source_fn<wall_time_point> wallClock() {
  return [](push_fn<wall_time_point> push) {
    return [push]() {
      push(std::chrono::system_clock::now());
    };
  };
}

#endif