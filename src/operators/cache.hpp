#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Wrapper.hpp>

namespace rheo {

  // Remember the last upstream value,
  // so it can be pulled any time you like.
  // This value will stick around and can be pulled
  // even after the upstream source is ended.
  // Note that if you use this with an async upstream source --
  // that is, it somehow pushes outside of control flow when pulled --
  // you may get weird double-push behaviour,
  // getting the cached value followed by the upstream value.

  template <typename T>
  source_fn<T> cache(source_fn<T> source) {
    return [source](push_fn<T> push, end_fn _) {
      auto lastSeenValue = std::make_shared<std::optional<T>>();
      auto isWithinPull = make_wrapper_shared(false);
      auto didPushWithinPull = make_wrapper_shared(false);

      pull_fn pull = source(
        [push, lastSeenValue, isWithinPull, didPushWithinPull](T value) {
          lastSeenValue->emplace(value);
          push(value);

          if (isWithinPull->value) {
            // This is being called as a direct consequence of a pull.
            // Set the flag that says that the push callback was called synchronously
            // so the pull function knows not to push the cached value.
            didPushWithinPull->value = true;
          }
        },
        [](){}
      );

      return [push, pull, lastSeenValue, isWithinPull, didPushWithinPull]() {
        // Set the flag that tells the upstream push callback
        // that it is being pushed because of a pull.
        isWithinPull->value = true;
        pull();

        // This assumes the upstream source is synchronous -- that is,
        // it has already called the push callback when we called `pull()`
        // (if it has a value to push, of course --
        // if it doesn't, that's the whole point of this operator).
        if (didPushWithinPull->value) {
          // Reset the flag for next pull.
          didPushWithinPull->value = false;
        } else if (lastSeenValue->has_value()) {
          // No sync push happened because of pull.
          // Push the cached value instead.
          push(lastSeenValue->value());
        }

        // Reset the flag, in case the push (or an unrelated push) is coming in async.
        isWithinPull->value = false;
      };
    };
  }

}