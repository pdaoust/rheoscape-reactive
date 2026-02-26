#pragma once

namespace rheoscape {

  template <typename T>
  struct Range {
    using value_type = T;
    T min;
    T max;

    Range(const T min, const T max) : min(min), max(max) { }
  };

}