#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  template <typename TClock>
  source_fn<typename TClock::time_point> fromClock() {
    return [](push_fn<typename TClock::time_point> push) {
      return [push = std::forward<push_fn<typename TClock::time_point>>(push)]() {
        push(std::move(TClock::now()));
      };
    };
  }

  template <typename TClock>
  map_fn<typename TClock::duration, typename TClock::time_point> toDuration = [](typename TClock::time_point value) { return value.time_since_epoch(); };

  template <typename TClock>
  map_fn<typename TClock::duration::rep, typename TClock::time_point> toRep = [](typename TClock::time_point value) { return value.time_since_epoch().count(); };

}