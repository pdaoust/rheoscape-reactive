#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>
#include <types/Wrapper.hpp>

namespace rheo::operators {

  // Concatenate two sources into one,
  // starting the second one after the first one has ended.
  template <typename T>
  source_fn<T> concat(source_fn<Endable<T>> source1, source_fn<T> source2) {
    return [source1, source2](push_fn<T> push) {
      auto source1HasEnded = make_wrapper_shared<bool>(false);
      pull_fn pullSource1 = source1([&push, source1HasEnded](Endable<T> value) {
        if (value.hasValue()) {
          push(value.value);
        } else {
          source1HasEnded->value = true;
        }
      });

      pull_fn pullSource2 = source2([&push, source1HasEnded](T value) {
        if (source1HasEnded->value) {
          push(value);
        }
      });

      return [source1HasEnded, pullSource1 = std::move(pullSource1), pullSource2 = std::move(pullSource2)]() {
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

  // Concatenate two sources into one,
  // starting the second one after the first one has ended.
  template <typename T>
  source_fn<Endable<T>> concat(source_fn<Endable<T>> source1, source_fn<Endable<T>> source2) {
    return [source1, source2](push_fn<T> push) {
      auto source1HasEnded = make_wrapper_shared<bool>(false);
      pull_fn pullSource1 = source1([push, source1HasEnded](Endable<T> value) {
        if (value.hasValue()) {
          push(value);
        } else {
          source1HasEnded->value = true;
        }
      });

      pull_fn pullSource2 = source2([push, source1HasEnded](T value) {
        if (source1HasEnded->value) {
          push(value);
        }
      });
      
      return [source1HasEnded, pullSource1, pullSource2]() {
        if (!source1HasEnded->value) {
          pullSource1();
        } else {
          pullSource2();
        }
      };
    };
  }

}