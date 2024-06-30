#ifndef RHEOSCAPE_ZIP
#define RHEOSCAPE_ZIP

#include <functional>
#include <memory>
#include <utility>
#include <core_types.hpp>

// Zip two streams together into one stream using a combining function.
// If you're using this in a push stream, it won't start emitting values
// until both sources have emitted a value.
// Thereafter, it'll emit a zipped value every time _either_ source emits a value,
// updating the respective portion of the zipped value.
// If you're using it in a pull stream, it'll pull both sources and combine them.
// An optional combiner function can be passed so you can combine the two values the way you like;
// the default just gloms them into a tuple.

template <typename TZipped, typename T1, typename T2>
source_fn<TZipped> zip_(
  source_fn<T1> source1,
  source_fn<T2> source2,
  combine2_fn<TZipped, T1, T2> combiner = [](T1 v1, T2 v2) { return std::tuple(v1, v2); }
) {
  return [source1, source2, combiner](push_fn<TZipped> push) {
    auto lastValue1 = std::make_shared<std::optional<T1>>();
    auto lastValue2 = std::make_shared<std::optional<T2>>();

    pull_fn pullSource1 = source1([combiner, push, lastValue1, lastValue2](T1 value) {
      lastValue1->emplace(value);
      if (lastValue2->has_value()) {
        push(combiner(value, lastValue2->value()));
      }
    });

    pull_fn pullSource2 = source2([combiner, push, lastValue1, lastValue2](T2 value) {
      lastValue2->emplace(value);
      if (lastValue1->has_value()) {
        push(combiner(lastValue1->value(), value));
      }
    });

    return [lastValue1, lastValue2, pullSource1, pullSource2]() {
      lastValue1->reset();
      lastValue2->reset();
      pullSource1();
      pullSource2();
    };
  };
}

template <typename TZipped, typename T1, typename T2>
pipe_fn<TZipped, T1> zip(
  source_fn<T2> source2,
  combine2_fn<TZipped, T1, T2> combiner = [](T1 v1, T2 v2) { return std::tuple(v1, v2); }
) {
  return [source2, combiner](source_fn<T1> source1) {
    return zip_(source1, source2, combiner);
  };
}

template <typename TZipped, typename T1, typename T2, typename T3>
source_fn<TZipped> zip_(
  source_fn<T1> source1,
  source_fn<T2> source2,
  source_fn<T3> source3,
  combine3_fn<TZipped, T1, T2, T3> combiner = [](T1 v1, T2 v2, T3 v3) { return std::tuple(v1, v2, v3); }
) {
  return [source1, source2, source3, combiner](push_fn<TZipped> push) {
    auto lastValue1 = std::make_shared<std::optional<T1>>();
    auto lastValue2 = std::make_shared<std::optional<T2>>();
    auto lastValue3 = std::make_shared<std::optional<T3>>();

    pull_fn pullSource1 = source1([combiner, lastValue1, lastValue2, lastValue3, push](T1 value) {
      lastValue1->emplace(value);
      if (lastValue2.has_value() && lastValue3.has_value()) {
        push(combiner(value, lastValue2.value(), lastValue3.value()));
      }
    });

    pull_fn pullSource2 = source2([combiner, lastValue1, lastValue2, lastValue3, push](T2 value) {
      lastValue2->emplace(value);
      if (lastValue1.has_value() && lastValue3.has_value()) {
        push(combiner(lastValue1.value(), value, lastValue3.value()));
      }
    });

    pull_fn pullSource3 = source3([combiner, lastValue1, lastValue2, lastValue3, push](T2 value) {
      lastValue3->emplace(value);
      if (lastValue1.has_value() && lastValue3.has_value()) {
        push(combiner(lastValue1.value(), lastValue2.value(), value));
      }
    });

    return [pullSource1, pullSource2, pullSource3, lastValue1, lastValue2, lastValue3]() {
      // Clear 'em both out so we get fresh values.
      lastValue1->reset();
      lastValue2->reset();
      lastValue3->reset();
      pullSource1();
      pullSource2();
      pullSource3();
    };
  };
}

template <typename TZipped, typename T1, typename T2, typename T3>
pipe_fn<TZipped, T1> zip(
  source_fn<T2> source2,
  source_fn<T3> source3,
  combine3_fn<TZipped, T1, T2, T3> combiner = [](T1 v1, T2 v2, T3 v3) { return std::tuple(v1, v2, v3); }
) {
  return [source2, source3, combiner](source_fn<T1> source1) {
    return zip_(source1, source2, source3, combiner);
  };
}

template <typename TZipped, typename T1, typename T2, typename T3, typename T4>
source_fn<TZipped> zip_(
  source_fn<T1> source1,
  source_fn<T2> source2,
  source_fn<T3> source3,
  source_fn<T4> source4,
  combine4_fn<TZipped, T1, T2, T3, T4> combiner = [](T1 v1, T2 v2, T3 v3, T4 v4) { return std::tuple(v1, v2, v3, v4); }
) {
  return [source1, source2, source3, source4, combiner](push_fn<TZipped> push) {
    auto lastValue1 = std::make_shared<std::optional<T1>>();
    auto lastValue2 = std::make_shared<std::optional<T2>>();
    auto lastValue3 = std::make_shared<std::optional<T3>>();
    auto lastValue4 = std::make_shared<std::optional<T4>>();

    pull_fn pullSource1 = source1([combiner, lastValue1, lastValue2, lastValue3, lastValue4, push](T1 value) {
      lastValue1->emplace(value);
      if (lastValue2.has_value() && lastValue3.has_value() && lastValue4.has_value()) {
        push(combiner(value, lastValue2.value(), lastValue3.value(), lastValue4.value()));
      }
    });

    pull_fn pullSource2 = source2([combiner, lastValue1, lastValue2, lastValue3, lastValue4, push](T2 value) {
      lastValue2->emplace(value);
      if (lastValue1.has_value() && lastValue3.has_value() && lastValue4.has_value()) {
        push(combiner(lastValue1.value(), value, lastValue3.value(), lastValue4.value()));
      }
    });

    pull_fn pullSource3 = source3([combiner, lastValue1, lastValue2, lastValue3, lastValue4, push](T3 value) {
      lastValue3->emplace(value);
      if (lastValue1.has_value() && lastValue3.has_value() && lastValue4.has_value()) {
        push(combiner(lastValue1.value(), lastValue2.value(), value, lastValue4.value()));
      }
    });

    pull_fn pullSource4 = source4([combiner, lastValue1, lastValue2, lastValue3, lastValue4, push](T4 value) {
      lastValue4->emplace(value);
      if (lastValue1.has_value() && lastValue2.has_value() && lastValue3.has_value()) {
        push(combiner(lastValue1.value(), lastValue2.value(), lastValue3.value(), value));
      }
    });

    return [pullSource1, pullSource2, pullSource3, pullSource4, lastValue1, lastValue2, lastValue3, lastValue4]() {
      // Clear 'em both out so we get fresh values.
      lastValue1->reset();
      lastValue2->reset();
      lastValue3->reset();
      lastValue4->reset();
      pullSource1();
      pullSource2();
      pullSource3();
      pullSource4();
    };
  };
}

template <typename TZipped, typename T1, typename T2, typename T3, typename T4>
pipe_fn<TZipped, T1> zip(
  source_fn<T2> source2,
  source_fn<T3> source3,
  source_fn<T4> source4,
  combine4_fn<TZipped, T1, T2, T3, T4> combiner = [](T1 v1, T2 v2, T3 v3, T4 v4) { return std::tuple(v1, v2, v3, v4); }
) {
  return [source2, source3, source4, combiner](source_fn<T1> source1) {
    return zip_(source1, source2, source3, source4, combiner);
  };
}

#endif