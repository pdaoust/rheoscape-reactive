#pragma once

namespace rheoscape {

  template <typename T>
  struct Range {
    using value_type = T;
    const T min;
    const T max;

    Range(const T min, const T max) : min(min), max(max) { }
  };

}