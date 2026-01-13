#pragma once

#include <functional>
#include <variant>
#include <core_types.hpp>

// Merge streams together.
// If you pull a merged stream, all the upstream sources get pulled in sequence.
// If one stream has ended, it'll still keep pushing values from the others.
// Chain multiple merge operations to merge more than two streams:
//   source1 | merge(source2) | merge(source3)

namespace rheo::operators {

  // Binary merge: combines two same-type sources into one stream
  template <typename T>
  struct merge_pull_handler {
    pull_fn pull1;
    pull_fn pull2;

    RHEO_NOINLINE void operator()() const {
      pull1();
      pull2();
    }
  };

  template <typename T>
  struct merge_source_binder {
    source_fn<T> source1;
    source_fn<T> source2;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      pull_fn pull1 = source1(push);
      pull_fn pull2 = source2(push);
      return merge_pull_handler<T>{std::move(pull1), std::move(pull2)};
    }
  };

  // Source function factory: merge two sources
  template <typename T>
  source_fn<T> merge(source_fn<T> source1, source_fn<T> source2) {
    return merge_source_binder<T>{std::move(source1), std::move(source2)};
  }

  // Pipe factory: source1 | merge(source2)
  template <typename T>
  pipe_fn<T, T> merge(source_fn<T> source2) {
    return [source2](source_fn<T> source1) {
      return merge(source1, source2);
    };
  }

  // Binary merge_mixed: combines two different-type sources into a variant stream
  template <typename T1, typename T2>
  struct merge_mixed_push_handler_1 {
    push_fn<std::variant<T1, T2>> push;

    RHEO_NOINLINE void operator()(T1 value) const {
      push(std::variant<T1, T2>(std::move(value)));
    }
  };

  template <typename T1, typename T2>
  struct merge_mixed_push_handler_2 {
    push_fn<std::variant<T1, T2>> push;

    RHEO_NOINLINE void operator()(T2 value) const {
      push(std::variant<T1, T2>(std::move(value)));
    }
  };

  template <typename T1, typename T2>
  struct merge_mixed_pull_handler {
    pull_fn pull1;
    pull_fn pull2;

    RHEO_NOINLINE void operator()() const {
      pull1();
      pull2();
    }
  };

  template <typename T1, typename T2>
  struct merge_mixed_source_binder {
    source_fn<T1> source1;
    source_fn<T2> source2;

    RHEO_NOINLINE pull_fn operator()(push_fn<std::variant<T1, T2>> push) const {
      pull_fn pull1 = source1(merge_mixed_push_handler_1<T1, T2>{push});
      pull_fn pull2 = source2(merge_mixed_push_handler_2<T1, T2>{push});
      return merge_mixed_pull_handler<T1, T2>{std::move(pull1), std::move(pull2)};
    }
  };

  // Source function factory: merge two different-type sources into variant
  template <typename T1, typename T2>
  source_fn<std::variant<T1, T2>> merge_mixed(source_fn<T1> source1, source_fn<T2> source2) {
    return merge_mixed_source_binder<T1, T2>{std::move(source1), std::move(source2)};
  }

  // Pipe factory: source1 | merge_mixed(source2)
  template <typename T1, typename T2>
  pipe_fn<std::variant<T1, T2>, T1> merge_mixed(source_fn<T2> source2) {
    return [source2](source_fn<T1> source1) {
      return merge_mixed(source1, source2);
    };
  }

}