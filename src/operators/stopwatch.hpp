#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/zip.hpp>

namespace rheo::operators {

  // Like `timestamp`, but resets the clock every time `lapCondition` is met.
  // It will glom contiguous values where `lapCondition` is met,
  // so you could use something like `temperature > 20.0f`
  // and it wouldn't restart the stopwatch at 0 for every value
  // while the temperature was above 20.

  template <typename TDuration, typename T, typename TTimePoint>
  source_fn<TaggedValue<T, TDuration>> stopwatch(source_fn<T> source, source_fn<TTimePoint> clockSource, filter_fn<T> lapCondition) {
    return zip(
      source,
      clockSource,
      // This is kinda sneaky. We're turning `zip` into a reducer here,
      // because the combining callback has state.
      (combine2_fn<TaggedValue<T, TDuration>, T, TTimePoint>)[lapCondition, lapStart = std::optional<TTimePoint>(), lastValueMatched = false](T value, TTimePoint ts) mutable {
        bool thisValueMatches = lapCondition(value);
        if (!lapStart.has_value() || (!lastValueMatched && thisValueMatches)) {
          // We've either just started the stream
          // or transitioned from a no-match state to a match state.
          lapStart = ts;
        }

        lastValueMatched = thisValueMatches;
        return TaggedValue(value, ts - lapStart.value());
      }
    );
  }

}