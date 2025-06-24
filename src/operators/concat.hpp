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
    return [source1, source2](push_fn<T> push, end_fn end) {
      auto source1HasEnded = make_wrapper_shared<bool>(false);
      pull_fn pullSource1 = source1(
        [push, source1HasEnded](Endable<T> value) {
          push(value.value);
          if (value.isLast) {
            source1HasEnded->value = true;
          }
        },
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

  // Concatenate two sources into one,
  // starting the second one after the first one has ended.
  template <typename T>
  source_fn<Endable<T>> concat(source_fn<Endable<T>> source1, source_fn<Endable<T>> source2, bool holdForSource2 = false) {
    return [source1, source2, holdForSource2](push_fn<T> push, end_fn end) {
      auto lastValueFromSource1 = std::make_shared<std::optional<T>>(std::nullopt);
      pull_fn pullSource1 = source1(
        [holdForSource2, push, lastValueFromSource1](Endable<T> value) {
          if (value.isLast) {
            lastValueFromSource1->emplace(std::optional<T>(value.value));
            if (holdForSource2) {
              // Don't push this value yet;
              // wait until we know whether there are any values to be found in source 2.
              return;
            }
            // Here, instead of just pushing the wrapped value,
            // we make sure the sink doesn't know it's the end.
            // Note that, if the second source is already ended,
            // we can't tell that to the sink because we don't know at this point.
            push(Endable<T>(value.value, false));
          } else {
            push(value);
          }
        },
        // This is incorrect behaviour, but end_fns are about to go away.
        [](){}
      );

      pull_fn pullSource2 = source2(
        [holdForSource2, push, lastValueFromSource1, lastValueFromSource1HasBeenPushed = false](T value) mutable {
          if (lastValueFromSource1->has_value()) {
            if (holdForSource2 && !lastValueFromSource1HasBeenPushed) {
              push(Endable<T>(lastValueFromSource1->value, false));
              lastValueFromSource1HasBeenPushed = true;
            }
            push(value);
          }
        },
        end
      );

      return [lastValueFromSource1, pullSource1, pullSource2]() {
        if (!lastValueFromSource1->has_value()) {
          pullSource1();
        } else {
          pullSource2();
        }
      };
    };
  }

}