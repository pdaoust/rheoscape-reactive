#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <types/Endable.hpp>
#include <types/Wrapper.hpp>
#include <operators/unwrap.hpp>

namespace rheoscape::operators {

  // Concatenate two sources into one,
  // starting the second one after the first one has ended.

  // First overload: Endable<T> + T -> T
  namespace detail {
    template <typename Source1T, typename Source2T>
    struct ConcatSourceBinder {
      using T = source_value_t<Source2T>;
      using value_type = T;

      Source1T source1;
      Source2T source2;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        auto source1HasEnded = make_wrapper_shared<bool>(false);

        struct Source1PushHandler {
          PushFn push;
          std::shared_ptr<Wrapper<bool>> source1HasEnded;

          RHEOSCAPE_CALLABLE void operator()(Endable<T> value) const {
            if (value.has_value()) {
              push(value.value);
            } else {
              source1HasEnded->value = true;
            }
          }
        };

        struct Source2PushHandler {
          PushFn push;
          std::shared_ptr<Wrapper<bool>> source1HasEnded;

          RHEOSCAPE_CALLABLE void operator()(T value) const {
            if (source1HasEnded->value) {
              push(value);
            }
          }
        };

        struct PullHandler {
          std::shared_ptr<Wrapper<bool>> source1HasEnded;
          pull_fn pull_source1;
          pull_fn pull_source2;

          RHEOSCAPE_CALLABLE void operator()() const {
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
  }

  template <typename Source1T, typename Source2T>
    requires concepts::Source<Source1T> && concepts::Source<Source2T> &&
             is_endable_v<source_value_t<Source1T>> &&
             std::is_same_v<endable_inner_t<source_value_t<Source1T>>, source_value_t<Source2T>> &&
             (!is_endable_v<source_value_t<Source2T>>)
  auto concat(Source1T source1, Source2T source2) {
    return detail::ConcatSourceBinder<Source1T, Source2T>{
      std::move(source1), std::move(source2)
    };
  }

  // Second overload: Endable<T> + Endable<T> -> Endable<T>
  namespace detail {
    template <typename Source1T, typename Source2T>
    struct ConcatEndableSourceBinder {
      using EndableT = source_value_t<Source1T>;
      using T = endable_inner_t<EndableT>;
      using value_type = Endable<T>;

      Source1T source1;
      Source2T source2;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        auto source1HasEnded = make_wrapper_shared<bool>(false);

        struct Source1PushHandler {
          PushFn push;
          std::shared_ptr<Wrapper<bool>> source1HasEnded;

          RHEOSCAPE_CALLABLE void operator()(Endable<T> value) const {
            if (value.has_value()) {
              push(value);
            } else {
              source1HasEnded->value = true;
            }
          }
        };

        struct Source2PushHandler {
          PushFn push;
          std::shared_ptr<Wrapper<bool>> source1HasEnded;

          RHEOSCAPE_CALLABLE void operator()(Endable<T> value) const {
            if (source1HasEnded->value) {
              push(value);
            }
          }
        };

        struct PullHandler {
          std::shared_ptr<Wrapper<bool>> source1HasEnded;
          pull_fn pull_source1;
          pull_fn pull_source2;

          RHEOSCAPE_CALLABLE void operator()() const {
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
  }

  template <typename Source1T, typename Source2T>
    requires concepts::Source<Source1T> && concepts::Source<Source2T> &&
             is_endable_v<source_value_t<Source1T>> && is_endable_v<source_value_t<Source2T>> &&
             std::is_same_v<source_value_t<Source1T>, source_value_t<Source2T>>
  auto concat(Source1T source1, Source2T source2) {
    return detail::ConcatEndableSourceBinder<Source1T, Source2T>{
      std::move(source1), std::move(source2)
    };
  }

  namespace detail {
    template <typename Source2T>
    struct ConcatPipeFactory {
      using is_pipe_factory = void;
      Source2T source2;

      template <typename Source1T>
        requires concepts::Source<Source1T>
      RHEOSCAPE_CALLABLE auto operator()(Source1T source1) const {
        return concat(std::move(source1), Source2T(source2));
      }
    };
  }

  template <typename Source2T>
    requires concepts::Source<Source2T>
  auto concat(Source2T source2) {
    return detail::ConcatPipeFactory<Source2T>{std::move(source2)};
  }

}
