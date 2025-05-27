#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>

namespace rheo::operators {

  template <typename T>
  source_fn<size_t> count(source_fn<T> source) {
    return [source](push_fn<size_t> push, end_fn end) {
      size_t i = 0;
      return source(
        [i, push](T value) mutable {
          i ++;
          push(i);
        },
        end
      );
    };
  }

  template <typename T>
  source_fn<TaggedValue<T, size_t>> tagCount(source_fn<T> source) {
    return [source](push_fn<TaggedValue<T, size_t>> push, end_fn end) {
      size_t i = 0;
      return source(
        [i, push](T value) mutable {
          i ++;
          push(TaggedValue<T, size_t> { value, i });
        },
        end
      );
    };
  }

}