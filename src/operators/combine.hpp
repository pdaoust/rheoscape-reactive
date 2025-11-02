#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <logging.hpp>
#include <core_types.hpp>
#include <types/Wrapper.hpp>
#include <operators/map.hpp>
#include <util.hpp>

namespace rheo::operators {

  // combine streams together into one stream using a combining function.
  // If you're using this in a push stream, it won't start emitting values
  // until both sources have emitted a value.
  // Thereafter, it'll emit a combined value every time _either_ source emits a value,
  // updating the respective portion of the combined value.
  // Make sure all the streams being combined
  // will push a value every time they're pulled,
  // because it will only push a value once both streams have pushed.
  // If you're using it in a pull stream, it'll pull both sources and combine them.
  // An optional combiner function can be passed so you can combine the two values the way you like;
  // the default just gloms them into a tuple.
  //
  // All of these sources end when any of their upstream sources ends.
  //
  // WORD OF CAUTION: If the first source is a source that pushes its first value on bind,
  // that first value will get missed because this operator needs to do some setup.

  template <typename CombineFn, typename T1, typename T2>
  auto combine(
    source_fn<T1> source1,
    source_fn<T2> source2,
    CombineFn&& combiner
  ) -> source_fn<transformer_2_in_out_type_t<CombineFn>> {
    using TOut = transformer_2_in_out_type_t<CombineFn>;

    return [source1, source2, combiner = std::forward<CombineFn>(combiner)](push_fn<TOut> push) {
      auto currentValue1 = std::make_shared<std::optional<T1>>();
      auto currentValue2 = std::make_shared<std::optional<T2>>();
      auto pullSource1 = std::make_shared<pull_fn>();
      // The second source's pull function won't be bound to anything when source 1 is ready;
      // if source 1 receives a value immediately upon subscribe,
      // source 2's pull would be a null pointer.
      // So we have to make it an optional and check for a value.
      auto pullSource2 = std::make_shared<std::optional<pull_fn>>();

      *pullSource1 = source1([combiner, push, currentValue1, currentValue2, pullSource2](T1&& value) {
        currentValue1->emplace(std::forward<T1>(value));
        if (currentValue2->has_value()) {
          // Source 2 has already been pushed to. Push and reset the values for next time.
          push(combiner(std::forward<T1>(currentValue1->value()), std::forward<T2>(currentValue2->value())));
          currentValue1->reset();
          currentValue2->reset();
        } else if (pullSource2->has_value()) {
          // We're going first. Pull the second source and let its push function handle the push.
          pullSource2->value()();
        }
      });

      pullSource2->emplace(source2([combiner, push, currentValue1, currentValue2, pullSource1](T2&& value) {
        currentValue2->emplace(std::forward<T2>(value));
        if (currentValue1->has_value()) {
          push(combiner(std::forward<T1>(currentValue1->value()), std::forward<T2>(currentValue2->value())));
          currentValue2->reset();
          currentValue1->reset();
        } else {
          (*pullSource1)();
        }
      }));

      return [pullSource1]() {
        // We only need to pull one source;
        // it'll handle the pulling of the other.
        (*pullSource1)();
      };
    };
  }

  template <typename CombineFn, typename T1, typename T2>
  auto combine(
    source_fn<T2> source2,
    CombineFn&& combiner
  ) -> pipe_fn<transformer_2_in_out_type_t<CombineFn>, T1> {
    using TOut = transformer_2_in_out_type_t<CombineFn>;
    return [source2, combiner = std::forward<CombineFn>(combiner)](source_fn<T1> source1) {
      return combine(source1, source2, combiner);
    };
  }

  template <typename CombineFn, typename T1, typename T2, typename T3>
  auto combine(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3,
    CombineFn&& combiner
  ) -> source_fn<transformer_3_in_out_type_t<CombineFn>> {
    using TOut = transformer_3_in_out_type_t<CombineFn>;

    return [source1, source2, source3, combiner = std::forward<CombineFn>(combiner)](push_fn<TOut> push) {
      auto currentValue1 = std::make_shared<std::optional<T1>>();
      auto currentValue2 = std::make_shared<std::optional<T2>>();
      auto currentValue3 = std::make_shared<std::optional<T3>>();
      auto pullSource1 = std::make_shared<pull_fn>();
      auto pullSource2 = std::make_shared<std::optional<pull_fn>>();
      auto pullSource3 = std::make_shared<std::optional<pull_fn>>();

      *pullSource1 = source1([combiner, push, currentValue1, currentValue2, currentValue3, pullSource2, pullSource3](T1&& value) {
        currentValue1->emplace(std::forward<T1>(value));
        if (currentValue2->has_value() && currentValue3->has_value()) {
          push(combiner(std::forward<T1>(currentValue1->value()), std::forward<T2>(currentValue2->value()), std::forward<T3>(currentValue3->value())));
          currentValue1->reset();
          currentValue2->reset();
          currentValue3->reset();
        } else if (pullSource2->has_value() && !currentValue2->has_value()) {
          // This will cascade to source 2, which will cascade to source 3 if needed.
          pullSource2->value()();
        } else if (pullSource3->has_value()) {
          pullSource3->value()();
        }
      });

      pullSource2->emplace(source2([combiner, push, currentValue1, currentValue2, currentValue3, pullSource1, pullSource3](T2&& value) {
        currentValue2->emplace(std::forward<T2>(value));
        if (currentValue1->has_value() && currentValue3->has_value()) {
          push(combiner(std::forward<T1>(currentValue1->value()), std::forward<T2>(currentValue2->value()), std::forward<T3>(currentValue3->value())));
          currentValue1->reset();
          currentValue2->reset();
          currentValue3->reset();
        } else if (!currentValue1->has_value()) {
          (*pullSource1)();
        } else if (pullSource3->has_value()) {
          pullSource3->value()();
        }
      }));

      pullSource3->emplace(source3([combiner, push, currentValue1, currentValue2, currentValue3, pullSource1, pullSource2](T3&& value) {
        currentValue3->emplace(std::forward<T3>(value));
        if (currentValue1->has_value() && currentValue2->has_value()) {
          push(combiner(std::forward<T1>(currentValue1->value()), std::forward<T2>(currentValue2->value()), std::forward<T3>(currentValue3->value())));
          currentValue1->reset();
          currentValue2->reset();
          currentValue3->reset();
        } else if (!currentValue1->has_value()) {
          (*pullSource1)();
        } else if (pullSource2->has_value() && !currentValue2->has_value()) {
          pullSource2->value()();
        }
        // We don't need to test for the existence of value 2 and pull from it,
        // because we know it'll cascade from 1 to 2.
      }));

      return [pullSource1]() {
        // We only need to pull one source;
        // it'll handle the pulling of the other.
        (*pullSource1)();
      };
    };
  }

  template <typename CombineFn, typename T1, typename T2, typename T3>
  auto combine(
    source_fn<T2> source2,
    source_fn<T3> source3,
    CombineFn&& combiner
  ) -> pipe_fn<transformer_3_in_out_type_t<CombineFn>, T1> {
    using TOut = transformer_3_in_out_type_t<CombineFn>;
    return [source2, source3, combiner = std::forward<CombineFn>(combiner)](source_fn<T1> source1) {
      return combine(std::forward<source_fn<T1>>(source1), std::forward<source_fn<T2>>(source2), std::forward<source_fn<T3>>(source3), std::forward<CombineFn>(combiner));
    };
  }

  template <typename CombineFn, typename T1, typename T2, typename T3, typename T4>
  auto combine(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4,
    CombineFn&& combiner
  ) -> source_fn<transformer_4_in_out_type_t<CombineFn>> {
    using TOut = transformer_4_in_out_type_t<CombineFn>;

    return [source1, source2, source3, source4, combiner = std::forward<CombineFn>(combiner)](push_fn<TOut> push) {
      auto currentValue1 = std::make_shared<std::optional<T1>>();
      auto currentValue2 = std::make_shared<std::optional<T2>>();
      auto currentValue3 = std::make_shared<std::optional<T3>>();
      auto currentValue4 = std::make_shared<std::optional<T4>>();
      auto pullSource1 = std::make_shared<pull_fn>();
      auto pullSource2 = std::make_shared<std::optional<pull_fn>>();
      auto pullSource3 = std::make_shared<std::optional<pull_fn>>();
      auto pullSource4 = std::make_shared<std::optional<pull_fn>>();

      *pullSource1 = source1([combiner, push, currentValue1, currentValue2, currentValue3, currentValue4, pullSource2, pullSource3, pullSource4](T1&& value) {
        currentValue1->emplace(std::forward<T1>(value));
        if (currentValue2->has_value()
          && currentValue3->has_value()
          && currentValue4->has_value()
        ) {
          push(combiner(
            std::forward<T1>(currentValue1->value()),
            std::forward<T2>(currentValue2->value()),
            std::forward<T3>(currentValue3->value()),
            std::forward<T4>(currentValue4->value())
          ));
          currentValue1->reset();
          currentValue2->reset();
          currentValue3->reset();
          currentValue4->reset();
        } else if (pullSource2->has_value() && !currentValue2->has_value()) {
          pullSource2->value()();
        } else if (pullSource3->has_value() && !currentValue3->has_value()) {
          pullSource3->value()();
        } else if (pullSource4->has_value()) {
          pullSource4->value()();
        }
      });

      pullSource2->emplace(source2([combiner, push, currentValue1, currentValue2, currentValue3, currentValue4, pullSource1, pullSource3, pullSource4](T2&& value) {
        currentValue2->emplace(std::forward<T2>(value));
        if (
          currentValue1->has_value()
          && currentValue3->has_value()
          && currentValue4->has_value()
        ) {
          push(combiner(
            std::forward<T1>(currentValue1->value()),
            std::forward<T2>(currentValue2->value()),
            std::forward<T3>(currentValue3->value()),
            std::forward<T4>(currentValue4->value())
          ));
          currentValue1->reset();
          currentValue2->reset();
          currentValue3->reset();
          currentValue4->reset();
        } else if (!currentValue1->has_value()) {
          (*pullSource1)();
        } else if (pullSource3->has_value() && !currentValue3->has_value()) {
          pullSource3->value()();
        } else if (pullSource4->has_value()) {
          pullSource4->value()();
        }
      }));

      pullSource3->emplace(source3([combiner, push, currentValue1, currentValue2, currentValue3, currentValue4, pullSource1, pullSource2, pullSource4](T3&& value) {
        currentValue3->emplace(std::forward<T3>(value));
        if (
          currentValue1->has_value()
          && currentValue2->has_value()
          && currentValue4->has_value()
        ) {
          push(combiner(
            std::forward<T1>(currentValue1->value()),
            std::forward<T2>(currentValue2->value()),
            std::forward<T3>(currentValue3->value()),
            std::forward<T4>(currentValue4->value())
          ));
          currentValue1->reset();
          currentValue2->reset();
          currentValue3->reset();
          currentValue4->reset();
        } else if (!currentValue1->has_value()) {
          (*pullSource1)();
        } else if (pullSource2->has_value() && !currentValue2->has_value()) {
          pullSource2->value()();
        } else if (pullSource4->has_value()) {
          pullSource4->value()();
        }
      }));

      pullSource4->emplace(source4([combiner, push, currentValue1, currentValue2, currentValue3, currentValue4, pullSource1, pullSource2, pullSource3](T4&& value) {
        currentValue4->emplace(std::forward<T4>(value));
        if (
          currentValue1->has_value()
          && currentValue2->has_value()
          && currentValue3->has_value()
        ) {
          push(combiner(
            std::forward<T1>(currentValue1->value()),
            std::forward<T2>(currentValue2->value()),
            std::forward<T3>(currentValue3->value()),
            std::forward<T4>(currentValue4->value())
          ));
          currentValue1->reset();
          currentValue2->reset();
          currentValue3->reset();
          currentValue4->reset();
        } else if (!currentValue1->has_value()) {
          (*pullSource1)();
        } else if (pullSource2->has_value() && !currentValue2->has_value()) {
          pullSource2->value()();
        } else if (pullSource3->has_value() && !currentValue3->has_value()) {
          pullSource3->value()();
        }
      }));

      return [pullSource1]() {
        // We only need to pull one source;
        // it'll handle the pulling of the other.
        (*pullSource1)();
      };
    };
  }

  template <typename CombineFn, typename T1, typename T2, typename T3, typename T4>
  auto combine(
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4,
    CombineFn&& combiner
  ) -> pipe_fn<transformer_4_in_out_type_t<CombineFn>, T1> {
    using TOut = transformer_4_in_out_type_t<CombineFn>;
    return [source2, source3, source4, combiner = std::forward<CombineFn>(combiner)](source_fn<T1> source1) {
      return combine(std::forward<source_fn<T1>>(source1), std::forward<source_fn<T2>>(source2), std::forward<source_fn<T3>>(source3), std::forward<source_fn<T4>>(source4), std::forward<CombineFn>(combiner));
    };
  }

}