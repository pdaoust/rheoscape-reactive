#ifndef RHEOSCAPE_EMPTY
#define RHEOSCAPE_EMPTY

#include <functional>
#include <core_types.hpp>

template <typename T>
source_fn<T> empty() {
  return [](push_fn<T> push, end_fn _) {
    return [](){};
  };
}

#endif