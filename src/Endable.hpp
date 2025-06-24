#pragma once

namespace rheo {

  template <typename T>
  struct Endable {
    const T value;
    const bool isLast;

    Endable(const T value, bool isLast = false)
    : value(value), isLast(isLast)
    { }
  };

}