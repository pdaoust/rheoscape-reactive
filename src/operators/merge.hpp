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

  namespace detail {
    template <typename Source1T, typename Source2T>
    struct MergeSourceBinder {
      static_assert(std::is_same_v<source_value_t<Source1T>, source_value_t<Source2T>>);
      using value_type = source_value_t<Source1T>;

      Source1T source1;
      Source2T source2;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {

        struct PullHandler {
          pull_fn pull1;
          pull_fn pull2;

          RHEO_CALLABLE void operator()() const {
            pull1();
            pull2();
          }
        };

        pull_fn pull1 = source1(push);
        pull_fn pull2 = source2(PushFn(push));
        return PullHandler{std::move(pull1), std::move(pull2)};
      }
    };
  }

  // Source function factory: merge two sources
  template <typename Source1T, typename Source2T>
    requires concepts::Source<Source1T> && concepts::Source<Source2T> &&
             std::is_same_v<source_value_t<Source1T>, source_value_t<Source2T>>
  auto merge(Source1T source1, Source2T source2) {
    return detail::MergeSourceBinder<Source1T, Source2T>{
      std::move(source1), std::move(source2)
    };
  }

  namespace detail {
    template <typename Source2T>
    struct MergePipeFactory {
      Source2T source2;

      template <typename Source1T>
        requires concepts::Source<Source1T> &&
                 std::is_same_v<source_value_t<Source1T>, source_value_t<Source2T>>
      RHEO_CALLABLE auto operator()(Source1T source1) const {
        return merge(std::move(source1), Source2T(source2));
      }
    };
  }

  // Pipe factory: source1 | merge(source2)
  template <typename Source2T>
    requires concepts::Source<Source2T>
  auto merge(Source2T source2) {
    return detail::MergePipeFactory<Source2T>{std::move(source2)};
  }

  // Binary merge_mixed: combines two different-type sources into a variant stream

  namespace detail {
    template <typename Source1T, typename Source2T>
    struct MergeMixedSourceBinder {
      using T1 = source_value_t<Source1T>;
      using T2 = source_value_t<Source2T>;
      using value_type = std::variant<T1, T2>;

      Source1T source1;
      Source2T source2;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler1 {
          PushFn push;

          RHEO_CALLABLE void operator()(T1 value) const {
            push(std::variant<T1, T2>(std::move(value)));
          }
        };

        struct PushHandler2 {
          PushFn push;

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
  }

  // Source function factory: merge two different-type sources into variant
  template <typename Source1T, typename Source2T>
    requires concepts::Source<Source1T> && concepts::Source<Source2T>
  auto merge_mixed(Source1T source1, Source2T source2) {
    return detail::MergeMixedSourceBinder<Source1T, Source2T>{
      std::move(source1), std::move(source2)
    };
  }

  namespace detail {
    template <typename Source2T>
    struct MergeMixedPipeFactory {
      Source2T source2;

      template <typename Source1T>
        requires concepts::Source<Source1T>
      RHEO_CALLABLE auto operator()(Source1T source1) const {
        return merge_mixed(std::move(source1), Source2T(source2));
      }
    };
  }

  // Pipe factory: source1 | merge_mixed(source2)
  template <typename Source2T>
    requires concepts::Source<Source2T>
  auto merge_mixed(Source2T source2) {
    return detail::MergeMixedPipeFactory<Source2T>{std::move(source2)};
  }

}
