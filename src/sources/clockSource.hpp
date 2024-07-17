#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>

namespace rheo {

  template <typename TClock>
  source_fn<typename TClock::time_point> clockSource() {
    return [](push_fn<typename TClock::time_point> push, end_fn _) {
      return [push]() {
        push(std::move(TClock::now()));
      };
    };
  }

}