#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/timestamp.hpp>

namespace rheo {
  
  // Don't emit a value until it's settled down
  // and matches the value recorded at the beginning of the interval.
  template <typename T, typename TTime, typename TInterval>
  source_fn<T> debounce(source_fn<T> source, source_fn<TTime> clockSource, TInterval interval) {
    auto timestamped = timestamp(source, clockSource);

    return [interval, timestamped](push_fn<T> push, end_fn end) {
      return timestamped(
        [
          interval,
          push,
          startState = std::optional<TSValue<TTime, T>>()
        ](TSValue<TTime, T> value) mutable {
          if (startState.has_value()) {
            if (value.timestamp - startState.value().timestamp > interval) {
              startState = std::nullopt;
              if (value.value == startState.value().value) {
                push(value.value);
              }
            }
          }

          if (!startState.has_value()) {
            startState = value;
          }
        },
        end
      );
    };
  }

  template <typename T, typename TTime, typename TInterval>
  pipe_fn<T, T> debounce(source_fn<TTime> clockSource, TInterval interval) {
    return [clockSource, interval](source_fn<T> source) {
      return debounce(source, clockSource, interval);
    };
  }

}