#ifndef RHEOSCAPE_END
#define RHEOSCAPE_END

#include <core_types.hpp>

// A source function that ends immediately.
template <typename T>
source_fn<T> end() {
  return [](push_fn<T> _, end_fn end) {
    return []() { end(); };
  };
}

#endif