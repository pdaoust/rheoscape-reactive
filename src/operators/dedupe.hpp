#ifndef RHEOSCAPE_DEDUPE
#define RHEOSCAPE_DEDUPE

#include <functional>
#include <core_types.hpp>

// Only push the first received instance of a value.
// There is no factory for this function because it already is a pipe function!
template <typename T>
source_fn<T> dedupe(source_fn<T> source) {
  return [source](push_fn<T> push) {
    std::optional<T> lastSeenValue;
    return source([push, &lastSeenValue](T value) {
      if (!lastSeenValue.has_value() || lastSeenValue.value() != value) {
        lastSeenValue = value;
        push(value);
      }
    });
  };
}

#endif