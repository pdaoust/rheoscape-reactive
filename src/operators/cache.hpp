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

  // Named callable for cache's push handler
  template<typename T>
  struct cache_push_handler {
    std::shared_ptr<push_fn<T>> sharedPush;
    std::shared_ptr<std::optional<T>> lastSeenValue;
    std::shared_ptr<bool> isWithinPull;
    std::shared_ptr<bool> didPushWithinPull;

    RHEO_NOINLINE void operator()(T value) const {
      lastSeenValue->emplace(value);
      (*sharedPush)(value);

      if (*isWithinPull) {
        // This is being called as a direct consequence of a pull.
        // Set the flag that says that the push callback was called synchronously
        // so the pull function knows not to push the cached value.
        *didPushWithinPull = true;
      }
    }
  };

  // Named callable for cache's pull function
  template<typename T>
  struct cache_pull_function {
    std::shared_ptr<push_fn<T>> sharedPush;
    pull_fn pull;
    std::shared_ptr<std::optional<T>> lastSeenValue;
    std::shared_ptr<bool> isWithinPull;
    std::shared_ptr<bool> didPushWithinPull;

    RHEO_NOINLINE void operator()() const {
      // Set the flag that tells the upstream push callback
      // that it is being pushed because of a pull.
      *isWithinPull = true;
      pull();

      // This assumes the upstream source is synchronous -- that is,
      // it has already called the push callback when we called `pull()`
      // (if it has a value to push, of course --
      // if it doesn't, that's the whole point of this operator).
      if (*didPushWithinPull) {
        // Reset the flag for next pull.
        *didPushWithinPull = false;
      } else if (lastSeenValue->has_value()) {
        // No sync push happened because of pull.
        // Push the cached value instead.
        (*sharedPush)(lastSeenValue->value());
      }

      // Reset the flag, in case the push (or an unrelated push) is coming in async.
      *isWithinPull = false;
    }
  };

  // Named callable for cache's source binder
  template<typename T>
  struct cache_source_binder {
    source_fn<T> source;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      auto sharedPush = std::make_shared<push_fn<T>>(push);
      auto lastSeenValue = std::make_shared<std::optional<T>>(std::nullopt);
      auto isWithinPull = std::make_shared<bool>(false);
      auto didPushWithinPull = std::make_shared<bool>(false);

      pull_fn pull = source(cache_push_handler<T>{sharedPush, lastSeenValue, isWithinPull, didPushWithinPull});

      return cache_pull_function<T>{sharedPush, pull, lastSeenValue, isWithinPull, didPushWithinPull};
    }
  };

  template <typename T>
  RHEO_INLINE source_fn<T> cache(source_fn<T> source) {
    return cache_source_binder<T>{source};
  }

  // Pipe factory for cache
  template<typename T>
  struct cache_pipe_factory {
    RHEO_NOINLINE source_fn<T> operator()(source_fn<T> source) const {
      return cache(std::move(source));
    }
  };

  template <typename T>
  pipe_fn<T, T> cache() {
    return cache_pipe_factory<T>{};
  }

}