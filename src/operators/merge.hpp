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

  // Source function factory: merge two sources
  template <typename T>
  source_fn<T> merge(source_fn<T> source1, source_fn<T> source2) {

    struct SourceBinder {
      source_fn<T> source1;
      source_fn<T> source2;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PullHandler {
          pull_fn pull1;
          pull_fn pull2;

          RHEO_CALLABLE void operator()() const {
            pull1();
            pull2();
          }
        };

        pull_fn pull1 = source1(push);
        pull_fn pull2 = source2(push);
        return PullHandler{std::move(pull1), std::move(pull2)};
      }
    };

    return SourceBinder{std::move(source1), std::move(source2)};
  }

  namespace detail {
    template <typename T>
    struct MergePipeFactory {
      source_fn<T> source2;

      RHEO_CALLABLE auto operator()(source_fn<T> source1) const {
        return merge(source1, source2);
      }
    };
  }

  // Pipe factory: source1 | merge(source2)
  template <typename T>
  auto merge(source_fn<T> source2) {
    return detail::MergePipeFactory<T>{source2};
  }

  // Binary merge_mixed: combines two different-type sources into a variant stream

  // Source function factory: merge two different-type sources into variant
  template <typename T1, typename T2>
  source_fn<std::variant<T1, T2>> merge_mixed(source_fn<T1> source1, source_fn<T2> source2) {

    struct SourceBinder {
      source_fn<T1> source1;
      source_fn<T2> source2;

      RHEO_CALLABLE pull_fn operator()(push_fn<std::variant<T1, T2>> push) const {

        struct PushHandler1 {
          push_fn<std::variant<T1, T2>> push;

          RHEO_CALLABLE void operator()(T1 value) const {
            push(std::variant<T1, T2>(std::move(value)));
          }
        };

        struct PushHandler2 {
          push_fn<std::variant<T1, T2>> push;

          RHEO_CALLABLE void operator()(T2 value) const {
            push(std::variant<T1, T2>(std::move(value)));
          }
        };

        struct PullHandler {
          pull_fn pull1;
          pull_fn pull2;

          RHEO_CALLABLE void operator()() const {
            pull1();
            pull2();
          }
        };

        pull_fn pull1 = source1(PushHandler1{push});
        pull_fn pull2 = source2(PushHandler2{push});
        return PullHandler{std::move(pull1), std::move(pull2)};
      }
    };

    return SourceBinder{std::move(source1), std::move(source2)};
  }

  namespace detail {
    template <typename T2>
    struct MergeMixedPipeFactory {
      source_fn<T2> source2;

      template <typename T1>
      RHEO_CALLABLE auto operator()(source_fn<T1> source1) const {
        return merge_mixed(source1, source2);
      }
    };
  }

  // Pipe factory: source1 | merge_mixed(source2)
  template <typename T2>
  auto merge_mixed(source_fn<T2> source2) {
    return detail::MergeMixedPipeFactory<T2>{source2};
  }

}
