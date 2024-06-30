#ifndef RHEOSCAPE_MERGE
#define RHEOSCAPE_MERGE

#include <functional>
#include <core_types.hpp>

// Don't use this with two pulls, or a push and a pull!
// In fact, it won't even let you pull.

template <typename T>
source_fn<T> merge(source_fn<T> source1, source_fn<T> source2) {
  return [source1, source2](push_fn<T> push, end_fn end) {
    auto source1HasEnded = std::make_shared<bool>();
    auto source2HasEnded = std::make_shared<bool>();
    source1(
      [push](T value) { push(value); },
      [end, source1HasEnded, source2HasEnded]() {
        source1HasEnded = true;
        if (source2HasEnded) {
          end();
        }
      }
    );
    source2(
      [push](T value) { push(value); },
      [end, source1HasEnded, source2HasEnded]() {
        source2HasEnded = true;
        if (source1HasEnded) {
          end();
        }
      }
    );
    return [](){};
  };
}

template <typename T>
pipe_fn<T, T> merge(source_fn<T> source2) {
  return [source2](source_fn<T> source1) {
    return merge(source1, source2);
  };
}

#endif