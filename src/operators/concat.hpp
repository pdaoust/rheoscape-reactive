#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>
#include <types/Wrapper.hpp>

namespace rheo::operators {

  // Concatenate two sources into one,
  // starting the second one after the first one has ended.

  // First overload: Endable<T> + T -> T
  template <typename T>
  source_fn<T> concat(source_fn<Endable<T>> source1, source_fn<T> source2) {

    struct SourceBinder {
      source_fn<Endable<T>> source1;
      source_fn<T> source2;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
        auto source1HasEnded = make_wrapper_shared<bool>(false);

        struct Source1PushHandler {
          push_fn<T> push;
          std::shared_ptr<Wrapper<bool>> source1HasEnded;

          RHEO_CALLABLE void operator()(Endable<T> value) const {
            if (value.has_value()) {
              push(value.value);
            } else {
              source1HasEnded->value = true;
            }
          }
        };

        struct Source2PushHandler {
          push_fn<T> push;
          std::shared_ptr<Wrapper<bool>> source1HasEnded;

          RHEO_CALLABLE void operator()(T value) const {
            if (source1HasEnded->value) {
              push(value);
            }
          }
        };

        struct PullHandler {
          std::shared_ptr<Wrapper<bool>> source1HasEnded;
          pull_fn pull_source1;
          pull_fn pull_source2;

          RHEO_CALLABLE void operator()() const {
            if (!source1HasEnded->value) {
              pull_source1();
            } else {
              pull_source2();
            }
          }
        };

        pull_fn pull_source1 = source1(Source1PushHandler{push, source1HasEnded});
        pull_fn pull_source2 = source2(Source2PushHandler{push, source1HasEnded});

        return PullHandler{
          source1HasEnded,
          std::move(pull_source1),
          std::move(pull_source2)
        };
      }
    };

    return SourceBinder{std::move(source1), std::move(source2)};
  }

  namespace detail {
    template <typename T>
    struct ConcatPipeFactory {
      source_fn<T> source2;

      // The piped source is source_fn<T>, and the existing source2 is also source_fn<T>.
      // For concat, the pipe version takes a source_fn<T> (not Endable),
      // but the original concat(source1, source2) signature expects Endable<T> for source1.
      // However, the existing pipe factory concat(source2) had signature pipe_fn<T, T>,
      // meaning it passed source_fn<T> directly. Looking at the original code,
      // it seems the pipe factory was for the case where both are source_fn<T>
      // and we don't actually use the Endable overload.
      // Let's preserve the original behavior.
      RHEO_CALLABLE auto operator()(source_fn<T> source1) const {
        return concat(source1, source2);
      }
    };
  }

  template <typename T>
  auto concat(source_fn<T> source2) {
    return detail::ConcatPipeFactory<T>{source2};
  }

  // Second overload: Endable<T> + Endable<T> -> Endable<T>
  template <typename T>
  source_fn<Endable<T>> concat(source_fn<Endable<T>> source1, source_fn<Endable<T>> source2) {

    struct SourceBinder {
      source_fn<Endable<T>> source1;
      source_fn<Endable<T>> source2;

      RHEO_CALLABLE pull_fn operator()(push_fn<Endable<T>> push) const {
        auto source1HasEnded = make_wrapper_shared<bool>(false);

        struct Source1PushHandler {
          push_fn<Endable<T>> push;
          std::shared_ptr<Wrapper<bool>> source1HasEnded;

          RHEO_CALLABLE void operator()(Endable<T> value) const {
            if (value.has_value()) {
              push(value);
            } else {
              source1HasEnded->value = true;
            }
          }
        };

        struct Source2PushHandler {
          push_fn<Endable<T>> push;
          std::shared_ptr<Wrapper<bool>> source1HasEnded;

          RHEO_CALLABLE void operator()(Endable<T> value) const {
            if (source1HasEnded->value) {
              push(value);
            }
          }
        };

        struct PullHandler {
          std::shared_ptr<Wrapper<bool>> source1HasEnded;
          pull_fn pull_source1;
          pull_fn pull_source2;

          RHEO_CALLABLE void operator()() const {
            if (!source1HasEnded->value) {
              pull_source1();
            }
            if (source1HasEnded->value) {
              // We can't use an else here;
              // it's possible that the above attempt to pull source 1
              // has resulted in the discovery that it's ended.
              pull_source2();
            }
          }
        };

        pull_fn pull_source1 = source1(Source1PushHandler{push, source1HasEnded});
        pull_fn pull_source2 = source2(Source2PushHandler{push, source1HasEnded});

        return PullHandler{
          source1HasEnded,
          std::move(pull_source1),
          std::move(pull_source2)
        };
      }
    };

    return SourceBinder{std::move(source1), std::move(source2)};
  }

}
