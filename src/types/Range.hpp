#pragma once

namespace rheo {

  template <typename T>
  struct Range {
    const T min;
    const T max;

    Range(const T min, const T max) : min(min), max(max) { }
  };

}