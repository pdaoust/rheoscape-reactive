#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Wrapper.hpp>

namespace rheo {

  // Concatenate two sources into one,
  // starting the second one after the first one has ended.
  template <typename T>
  source_fn<T> concat(source_fn<T> source1, source_fn<T> source2) {
    return [source1, source2](push_fn<T> push, end_fn end) {
      auto source1HasEnded = make_wrapper_shared<bool>(false);
      pull_fn pullSource1 = source1(
        [push](T value) { push(value); },
        [source1HasEnded]() {
          source1HasEnded->value = true;
        }
      );
      pull_fn pullSource2 = source2(
        [push, source1HasEnded](T value) {
          if (source1HasEnded->value) {
            push(value);
          }
        },
        end
      );
      return [source1HasEnded, pullSource1, pullSource2]() {
        if (!source1HasEnded->value) {
          pullSource1();
        } else {
          pullSource2();
        }
      };
    };
  }

  template <typename T>
  pipe_fn<T, T> concat(source_fn<T> source2) {
    return [source2](source_fn<T> source1) {
      return concat(source1, source2);
    };
  }

}