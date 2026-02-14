#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  // Remember the last upstream value,
  // so it can be pulled any time you like.
  // This value will stick around and can be pulled
  // even after the upstream source is ended.
  // Note that if you use this with an async upstream source --
  // that is, it somehow pushes outside of control flow when pulled --
  // you may get weird double-push behaviour,
  // getting the cached value followed by the upstream value.
  // (Currently this doesn't exist because everything in rheoscape is single-threaded.)

  namespace detail {
    template <typename T>
    struct CacheSourceBinder {
      using value_type = T;

      source_fn<T> source;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
        auto shared_push = std::make_shared<push_fn<T>>(push);
        auto last_seen_value = std::make_shared<std::optional<T>>(std::nullopt);
        auto is_within_pull = std::make_shared<bool>(false);
        auto did_push_within_pull = std::make_shared<bool>(false);

        struct PushHandler {
          std::shared_ptr<push_fn<T>> shared_push;
          std::shared_ptr<std::optional<T>> last_seen_value;
          std::shared_ptr<bool> is_within_pull;
          std::shared_ptr<bool> did_push_within_pull;

          RHEO_CALLABLE void operator()(T value) const {
            last_seen_value->emplace(value);
            (*shared_push)(value);

            if (*is_within_pull) {
              // This is being called as a direct consequence of a pull.
              // Set the flag that says that the push callback was called synchronously
              // so the pull function knows not to push the cached value.
              *did_push_within_pull = true;
            }
          }
        };

        struct PullFunction {
          std::shared_ptr<push_fn<T>> shared_push;
          pull_fn pull;
          std::shared_ptr<std::optional<T>> last_seen_value;
          std::shared_ptr<bool> is_within_pull;
          std::shared_ptr<bool> did_push_within_pull;

          RHEO_CALLABLE void operator()() const {
            // Set the flag that tells the upstream push callback
            // that it is being pushed because of a pull.
            *is_within_pull = true;
            pull();

            // This assumes the upstream source is synchronous -- that is,
            // it has already called the push callback when we called `pull()`
            // (if it has a value to push, of course --
            // if it doesn't, that's the whole point of this operator).
            if (*did_push_within_pull) {
              // Reset the flag for next pull.
              *did_push_within_pull = false;
            } else if (last_seen_value->has_value()) {
              // No sync push happened because of pull.
              // Push the cached value instead.
              (*shared_push)(last_seen_value->value());
            }

            // Reset the flag, in case the push (or an unrelated push) is coming in async.
            *is_within_pull = false;
          }
        };

        pull_fn pull = source(PushHandler{shared_push, last_seen_value, is_within_pull, did_push_within_pull});

        return PullFunction{shared_push, pull, last_seen_value, is_within_pull, did_push_within_pull};
      }
    };
  }

  template <typename SourceT>
    requires concepts::Source<SourceT>
  RHEO_CALLABLE auto cache(SourceT source) {
    using T = source_value_t<SourceT>;
    return detail::CacheSourceBinder<T>{source_fn<T>(std::move(source))};
  }

  namespace detail {
    struct CachePipeFactory {
      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEO_CALLABLE auto operator()(SourceT source) const {
        return cache(std::move(source));
      }
    };
  }

  inline auto cache() {
    return detail::CachePipeFactory{};
  }

}
