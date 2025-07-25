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

    return [&source1, &source2, combiner = std::forward<CombineFn>(combiner)](push_fn<TOut> push) {
      auto currentValue1 = std::make_shared<std::optional<T1>>();
      auto currentValue2 = std::make_shared<std::optional<T2>>();

      pull_fn pullSource1 = source1([combiner, &push, currentValue1, currentValue2](T1&& value) {
        currentValue1->emplace(std::forward<T1>(value));
        if (currentValue2->has_value()) {
          push(combiner(std::forward<T1>(currentValue1->value()), std::forward<T2>(currentValue2->value())));
        }
      });

      pull_fn pullSource2 = source2([&combiner, &push, currentValue1, currentValue2](T2&& value) {
        currentValue2->emplace(std::forward<T2>(value));
        if (currentValue1->has_value()) {
          push(combiner(std::forward<T1>(currentValue1->value()), std::forward<T2>(currentValue2->value())));
        }
      });

      return [pullSource1 = std::move(pullSource1), pullSource2 = std::move(pullSource2)]() {
        // FIXME: Should it pull both sources? Maybe all we want is to pull the first one?
        // This is necessary first time round, but after that it'll cause the combined value to be pushed twice.
        // On the other hand, if we don't pull both sources,
        // the second source will stay stale forevermore.
        pullSource1();
        pullSource2();
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

      pull_fn pullSource1 = source1([&combiner, &push, currentValue1, currentValue2, currentValue3](T1 value) {
        currentValue1->emplace(value);
        if (currentValue2->has_value() && currentValue3->has_value()) {
          push(combiner(value, currentValue2->value(), currentValue3->value()));
        }
      });

      pull_fn pullSource2 = source2([&combiner, &push, currentValue1, currentValue2, currentValue3](T2 value) {
        currentValue2->emplace(value);
        if (currentValue1->has_value() && currentValue3->has_value()) {
          push(combiner(currentValue1->value(), value, currentValue3->value()));
        }
      });

      pull_fn pullSource3 = source3([&combiner, &push, currentValue1, currentValue2, currentValue3](T3 value) {
        currentValue3->emplace(value);
        if (currentValue1->has_value() && currentValue2->has_value()) {
          push(combiner(currentValue1->value(), currentValue2->value(), value));
        }
      });

      return [pullSource1 = std::move(pullSource1), pullSource2 = std::move(pullSource2), pullSource3 = std::move(pullSource3)]() {
        pullSource1();
        pullSource2();
        pullSource3();
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

      pull_fn pullSource1 = source1([&combiner, &push, currentValue1, currentValue2, currentValue3, currentValue4](T1 value) {
        currentValue1->emplace(value);
        if (currentValue2->has_value() && currentValue3->has_value()) {
          push(combiner(value, currentValue2->value(), currentValue3->value(), currentValue4->value()));
        }
      });

      pull_fn pullSource2 = source2([&combiner, &push, currentValue1, currentValue2, currentValue3, currentValue4](T2 value) {
        currentValue2->emplace(value);
        if (currentValue1->has_value() && currentValue3->has_value()) {
          push(combiner(currentValue1->value(), value, currentValue3->value(), currentValue4->value()));
        }
      });

      pull_fn pullSource3 = source3([&combiner, &push, currentValue1, currentValue2, currentValue3, currentValue4](T3 value) {
        currentValue3->emplace(value);
        if (currentValue1->has_value() && currentValue2->has_value()) {
          push(combiner(currentValue1->value(), currentValue2->value(), value, currentValue4->value()));
        }
      });

      pull_fn pullSource4 = source4([&combiner, &push, currentValue1, currentValue2, currentValue3, currentValue4](T4 value) {
        currentValue4->emplace(value);
        if (currentValue1->has_value() && currentValue2->has_value() && currentValue3->has_value()) {
          push(combiner(currentValue1->value(), currentValue2->value(), currentValue3->value(), value));
        }
      });

      return [pullSource1 = std::move(pullSource1), pullSource2 = std::move(pullSource2), pullSource3 = std::move(pullSource3), pullSource4 = std::move(pullSource4)]() {
        pullSource1();
        pullSource2();
        pullSource3();
        pullSource4();
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