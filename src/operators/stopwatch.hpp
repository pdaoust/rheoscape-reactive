#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>

namespace rheo::operators {

  // Like `timestamp`, but resets the clock every time `lapCondition` is met.
  // It will glom contiguous values where `lapCondition` is met,
  // so you could use something like `temperature > 20.0f`
  // and it wouldn't restart the stopwatch at 0 for every value
  // while the temperature was above 20.

  template <typename TDuration, typename T, typename TTimePoint, typename FilterFn>
  source_fn<TaggedValue<T, TDuration>> stopwatch(source_fn<T> source, source_fn<TTimePoint> clockSource, FilterFn&& lapCondition) {
    return combine(
      // This is kinda sneaky. We're turning `combine` into a reducer here,
      // because the combining callback has state.
      (combine2_fn<TaggedValue<T, TDuration>, T, TTimePoint>)[lapCondition = std::forward<FilterFn>(lapCondition), lapStart = std::optional<TTimePoint>(), lastValueMatched = false](T value, TTimePoint ts) mutable {
        bool thisValueMatches = lapCondition(value);
        if (!lapStart.has_value() || (!lastValueMatched && thisValueMatches)) {
          // We've either just started the stream
          // or transitioned from a no-match state to a match state.
          lapStart = ts;
        }

        lastValueMatched = thisValueMatches;
        return TaggedValue(value, ts - lapStart.value());
      },
      source,
      clockSource
    );
  }

  // Pipe factory
  template <typename TDuration, typename TTimePoint, typename FilterFn>
  auto stopwatch(source_fn<TTimePoint> clockSource, FilterFn&& lapCondition)
  -> pipe_fn<TaggedValue<arg_of<FilterFn>, TDuration>, arg_of<FilterFn>> {
    using T = arg_of<FilterFn>;
    return [clockSource, lapCondition = std::forward<FilterFn>(lapCondition)](source_fn<T> source) -> source_fn<TaggedValue<T, TDuration>> {
      return stopwatch<TDuration>(std::move(source), clockSource, std::move(lapCondition));
    };
  }

}