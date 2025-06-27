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

  template <typename TCombined, typename T1, typename T2>
  source_fn<TCombined> combine(
    source_fn<T1> source1,
    source_fn<T2> source2,
    combine2_fn<TCombined, T1, T2> combiner
  ) {
    return [source1, source2, combiner](push_fn<TCombined> push) {
      auto currentValue1 = std::make_shared<std::optional<T1>>();
      auto currentValue2 = std::make_shared<std::optional<T2>>();

      auto pullSource1 = source1([combiner, push, currentValue1, currentValue2](T1 value) {
        currentValue1->emplace(value);
        if (currentValue2->has_value()) {
          push(combiner(value, currentValue2->value()));
        }
      });

      auto pullSource2 = source2([combiner, push, currentValue1, currentValue2](T2 value) {
        currentValue2->emplace(value);
        if (currentValue1->has_value()) {
          push(combiner(currentValue1->value(), value));
        }
      });

      return [pullSource1, pullSource2]() {
        pullSource1();
        pullSource2();
      };
    };
  }

  template <typename T1, typename T2>
  source_fn<std::tuple<T1, T2>> combineTuple(source_fn<T1> source1, source_fn<T2> source2) {
    return combine<std::tuple<T1, T2>, T1, T2>(source1, source2, [](T1 v1, T2 v2) { return std::tuple(v1, v2); });
  }

  template <typename TCombined, typename T1, typename T2>
  pipe_fn<TCombined, T1> combine(
    source_fn<T2> source2,
    combine2_fn<TCombined, T1, T2> combiner
  ) {
    return [source2, combiner](source_fn<T1> source1) {
      return combine(source1, source2, combiner);
    };
  }

  template <typename T1, typename T2>
  pipe_fn<std::tuple<T1, T2>, T1> combineTuple(
    source_fn<T2> source2
  ) {
    return [source2](source_fn<T1> source1) {
      return combineTuple(source1, source2);
    };
  }

  template <typename TCombined, typename T1, typename T2, typename T3>
  source_fn<TCombined> combine3(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3,
    combine3_fn<TCombined, T1, T2, T3> combiner
  ) {
    return map<TCombined, std::tuple<std::tuple<T1, T2>, T3>>(
      combineTuple(combineTuple(source1, source2), source3),
      (map_fn<TCombined, std::tuple<std::tuple<T1, T2>, T3>>)[combiner](std::tuple<std::tuple<T1, T2>, T3> value) {
        return combiner(
          std::get<0>(std::get<0>(value)),
          std::get<1>(std::get<0>(value)),
          std::get<1>(value)
        );
      }
    );
  }

  template <typename T1, typename T2, typename T3>
  source_fn<std::tuple<T1, T2, T3>> combine3Tuple(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3
  ) {
    return combine3<std::tuple<T1, T2, T3>, T1, T2, T3>(
      source1,
      source2,
      source3,
      [](T1 v1, T2 v2, T3 v3) { return std::tuple(v1, v2, v3); }
    );
  }

  template <typename TCombined, typename T1, typename T2, typename T3>
  pipe_fn<TCombined, T1> combine3(
    source_fn<T2> source2,
    source_fn<T3> source3,
    combine3_fn<TCombined, T1, T2, T3> combiner = [](T1 v1, T2 v2, T3 v3) { return std::tuple(v1, v2, v3); }
  ) {
    return [source2, source3, combiner](source_fn<T1> source1) {
      return combine3(source1, source2, source3, combiner);
    };
  }

  template <typename T1, typename T2, typename T3>
  pipe_fn<std::tuple<T1, T2, T3>, T1> combine3Tuple(
    source_fn<T2> source2,
    source_fn<T3> source3
  ) {
    return [source2, source3](source_fn<T1> source1) {
      return combineTuple(source1, source2, source3);
    };
  }

  template <typename TCombined, typename T1, typename T2, typename T3, typename T4>
  source_fn<TCombined> combine4(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4,
    combine4_fn<TCombined, T1, T2, T3, T4> combiner
  ) {
    return map<TCombined, std::tuple<std::tuple<std::tuple<T1, T2>, T3>, T4>>(
      combineTuple(combineTuple(combineTuple(source1, source2), source3), source4),
      (map_fn<TCombined, std::tuple<std::tuple<std::tuple<T1, T2>, T3>, T4>>)[combiner](std::tuple<std::tuple<std::tuple<T1, T2>, T3>, T4> value) {
        return combiner(
          std::get<0>(std::get<0>(std::get<0>(value))),
          std::get<1>(std::get<0>(std::get<0>(value))),
          std::get<1>(std::get<0>(value)),
          std::get<1>(value)
        );
      }
    );
  }

  template <typename T1, typename T2, typename T3, typename T4>
  source_fn<std::tuple<T1, T2, T3, T4>> combine4Tuple(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4
  ) {
    return combine4<std::tuple<T1, T2, T3, T4>, T1, T2, T3, T4>(
      source1,
      source2,
      source3,
      source4,
      [](T1 v1, T2 v2, T3 v3, T4 v4) { return std::tuple(v1, v2, v3, v4); }
    );
  }

  template <typename TCombined, typename T1, typename T2, typename T3, typename T4>
  pipe_fn<TCombined, T1> combine4(
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4,
    combine4_fn<TCombined, T1, T2, T3, T4> combiner = [](T1 v1, T2 v2, T3 v3, T4 v4) { return std::tuple(v1, v2, v3, v4); }
  ) {
    return [source2, source3, source4, combiner](source_fn<T1> source1) {
      return combine4(source1, source2, source3, source4, combiner);
    };
  }

  template <typename T1, typename T2, typename T3, typename T4>
  pipe_fn<std::tuple<T1, T2, T3, T4>, T1> combine4Tuple(
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4
  ) {
    return [source2, source3, source4](source_fn<T1> source1) {
      return combine4Tuple(source1, source2, source3, source4);
    };
  }

}