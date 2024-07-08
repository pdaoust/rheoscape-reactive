#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/zip.hpp>

namespace rheo {
  
  // Only allow at most one value per interval,
  // dropping everything in between.
  template <typename T, typename TTime, typename TInterval>
  source_fn<T> throttle(source_fn<T> source, source_fn<TTime> clockSource, TInterval interval) {
    auto timestamped = timestamp(source, clockSource);

    return [interval, timestamped](push_fn<T> push, end_fn end) {
      return timestamped(
        [interval, push, intervalStart = std::optional<TTime>()](TaggedValue<TTime, T> value) mutable {
          if (intervalStart.has_value() && value.tag - intervalStart.value() > interval) {
            intervalStart = std::nullopt;
          }

          if (!intervalStart.has_value()) {
            intervalStart = value.tag;
            push(value);
          }
        },
        end
      );
    };
  }

  template <typename T, typename TTime, typename TInterval>
  pipe_fn<T, T> throttle(source_fn<TTime> clockSource, TInterval interval) {
    return [clockSource, interval](source_fn<T> source) {
      return throttle(source, clockSource, interval);
    };
  }

}