#pragma once

namespace rheoscape {

  template <typename T>
  struct Range {
    using value_type = T;
    T min;
    T max;

    Range(const T min, const T max) : min(min), max(max) { }

    bool operator==(Range<T> other) {
      return min == other.min && max == other.max;
    }

    bool contains(T value) {
      if (min == max) {
        return value == min;
      }
      if (min < max) {
        return min <= value && value <= max;
      }
      return min >= value && value >= max;
    }

    bool contains(Range<T> value) {
      return contains(value.min) && contains(value.max);
    }

    bool overlaps(Range<T> value) {
      return contains(value.min) || contains(value.max);
    }
  };

}