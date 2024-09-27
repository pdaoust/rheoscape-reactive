#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <core_types.hpp>
#include <operators/map.hpp>
#include <util.hpp>

namespace rheo {

  // Zip streams together into one stream using a combining function.
  // If you're using this in a push stream, it won't start emitting values
  // until both sources have emitted a value.
  // Thereafter, it'll emit a zipped value every time _either_ source emits a value,
  // updating the respective portion of the zipped value.
  // Make sure all the streams being zippped
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

  template <typename TZipped, typename T1, typename T2>
  source_fn<TZipped> zip(
    source_fn<T1> source1,
    source_fn<T2> source2,
    combine2_fn<TZipped, T1, T2> combiner
  ) {
    return [source1, source2, combiner](push_fn<TZipped> push, end_fn end) {
      auto lastValue1 = std::make_shared<std::optional<T1>>();
      auto lastValue2 = std::make_shared<std::optional<T2>>();
      auto pullSource1 = std::make_shared<std::optional<pull_fn>>();
      auto pullSource2 = std::make_shared<std::optional<pull_fn>>();
      auto endAny = std::make_shared<EndAny>(end);

      pullSource1->emplace(source1(
        [combiner, push, lastValue1, lastValue2, pullSource2, endAny](T1 value) {
          lastValue1->emplace(value);
          if (pullSource2->has_value() && !lastValue2->has_value()) {
            pullSource2->value()();
          } else if (!endAny->ended && lastValue2->has_value()) {
            push(combiner(value, lastValue2->value()));
          }
        },
        endAny->upstream_end_fn
      )); 

      pullSource2->emplace(source2(
        [combiner, push, lastValue1, lastValue2, pullSource1, endAny](T2 value) {
          lastValue2->emplace(value);
          if (pullSource1->has_value() && !lastValue1->has_value()) {
            pullSource1->value()();
          } else if (!endAny->ended && lastValue1->has_value()) {
            push(combiner(lastValue1->value(), value));
          }
        },
        endAny->upstream_end_fn
      ));

      return [lastValue1, lastValue2, pullSource1, pullSource2]() {
        lastValue1->reset();
        lastValue2->reset();
        // We don't need to pull from both; pulling from #1 will pull from #2.
        pullSource1->value()();
      };
    };
  }

  template <typename T1, typename T2>
  source_fn<std::tuple<T1, T2>> zipTuple(source_fn<T1> source1, source_fn<T2> source2) {
    return zip<std::tuple<T1, T2>, T1, T2>(source1, source2, [](T1 v1, T2 v2) { return std::tuple(v1, v2); });
  }

  template <typename TZipped, typename T1, typename T2>
  pipe_fn<TZipped, T1> zip(
    source_fn<T2> source2,
    combine2_fn<TZipped, T1, T2> combiner
  ) {
    return [source2, combiner](source_fn<T1> source1) {
      return zip(source1, source2, combiner);
    };
  }

  template <typename T1, typename T2>
  pipe_fn<std::tuple<T1, T2>, T1> zipTuple(
    source_fn<T2> source2
  ) {
    return [source2](source_fn<T1> source1) {
      return zipTuple(source1, source2);
    };
  }

  // map(
  //   rheo::source_fn<
  //     std::tuple<
  //       std::tuple<int, char>,
  //       bool>
  //     >,
  //     rheo::zip<
  //       std::tuple<int, char, bool>,
  //       int,
  //       char,
  //       bool
  //     >(
  //       source_fn<int>,
  //       source_fn<char>,
  //       source_fn<bool>,
  //       combine3_fn<std::tuple<int, char, bool>, int, char, bool>
  //     )::<lambda(std::tuple<std::tuple<int, char>, bool>)>
  //   )

  template <typename TZipped, typename T1, typename T2, typename T3>
  source_fn<TZipped> zip3(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3,
    combine3_fn<TZipped, T1, T2, T3> combiner
  ) {
    return map<TZipped, std::tuple<std::tuple<T1, T2>, T3>>(
      zipTuple(zipTuple(source1, source2), source3),
      (map_fn<TZipped, std::tuple<std::tuple<T1, T2>, T3>>)[combiner](std::tuple<std::tuple<T1, T2>, T3> value) {
        return combiner(
          std::get<0>(std::get<0>(value)),
          std::get<1>(std::get<0>(value)),
          std::get<1>(value)
        );
      }
    );
  }

  template <typename T1, typename T2, typename T3>
  source_fn<std::tuple<T1, T2, T3>> zip3Tuple(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3
  ) {
    return zip3<std::tuple<T1, T2, T3>, T1, T2, T3>(
      source1,
      source2,
      source3,
      [](T1 v1, T2 v2, T3 v3) { return std::tuple(v1, v2, v3); }
    );
  }

  template <typename TZipped, typename T1, typename T2, typename T3>
  pipe_fn<TZipped, T1> zip3(
    source_fn<T2> source2,
    source_fn<T3> source3,
    combine3_fn<TZipped, T1, T2, T3> combiner = [](T1 v1, T2 v2, T3 v3) { return std::tuple(v1, v2, v3); }
  ) {
    return [source2, source3, combiner](source_fn<T1> source1) {
      return zip3(source1, source2, source3, combiner);
    };
  }

  template <typename T1, typename T2, typename T3>
  pipe_fn<std::tuple<T1, T2, T3>, T1> zip3Tuple(
    source_fn<T2> source2,
    source_fn<T3> source3
  ) {
    return [source2, source3](source_fn<T1> source1) {
      return zipTuple(source1, source2, source3);
    };
  }

  template <typename TZipped, typename T1, typename T2, typename T3, typename T4>
  source_fn<TZipped> zip4(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4,
    combine4_fn<TZipped, T1, T2, T3, T4> combiner
  ) {
    return map<TZipped, std::tuple<std::tuple<std::tuple<T1, T2>, T3>, T4>>(
      zipTuple(zipTuple(zipTuple(source1, source2), source3), source4),
      (map_fn<TZipped, std::tuple<std::tuple<std::tuple<T1, T2>, T3>, T4>>)[combiner](std::tuple<std::tuple<std::tuple<T1, T2>, T3>, T4> value) {
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
  source_fn<std::tuple<T1, T2, T3, T4>> zip4Tuple(
    source_fn<T1> source1,
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4
  ) {
    return zip4<std::tuple<T1, T2, T3, T4>, T1, T2, T3, T4>(
      source1,
      source2,
      source3,
      source4,
      [](T1 v1, T2 v2, T3 v3, T4 v4) { return std::tuple(v1, v2, v3, v4); }
    );
  }

  template <typename TZipped, typename T1, typename T2, typename T3, typename T4>
  pipe_fn<TZipped, T1> zip4(
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4,
    combine4_fn<TZipped, T1, T2, T3, T4> combiner = [](T1 v1, T2 v2, T3 v3, T4 v4) { return std::tuple(v1, v2, v3, v4); }
  ) {
    return [source2, source3, source4, combiner](source_fn<T1> source1) {
      return zip4(source1, source2, source3, source4, combiner);
    };
  }

  template <typename T1, typename T2, typename T3, typename T4>
  pipe_fn<std::tuple<T1, T2, T3, T4>, T1> zip4Tuple(
    source_fn<T2> source2,
    source_fn<T3> source3,
    source_fn<T4> source4
  ) {
    return [source2, source3, source4](source_fn<T1> source1) {
      return zip4Tuple(source1, source2, source3, source4);
    };
  }

}