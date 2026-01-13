#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>

namespace rheo::operators {

  // Like `timestamp`, but resets the clock every time `lap_condition` is met.
  // It will glom contiguous values where `lap_condition` is met,
  // so you could use something like `temperature > 20.0f`
  // and it wouldn't restart the stopwatch at 0 for every value
  // while the temperature was above 20.

  template <typename TDuration, typename T, typename TTimePoint, typename FilterFn>
  source_fn<TaggedValue<T, TDuration>> stopwatch(source_fn<T> source, source_fn<TTimePoint> clock_source, FilterFn&& lap_condition) {
    return combine(
      // This is kinda sneaky. We're turning `combine` into a reducer here,
      // because the combining callback has state.
      (combine2_fn<TaggedValue<T, TDuration>, T, TTimePoint>)[lap_condition = std::forward<FilterFn>(lap_condition), lap_start = std::optional<TTimePoint>(), last_value_matched = false](T value, TTimePoint ts) mutable {
        bool this_value_matches = lap_condition(value);
        if (!lap_start.has_value() || (!last_value_matched && this_value_matches)) {
          // We've either just started the stream
          // or transitioned from a no-match state to a match state.
          lap_start = ts;
        }

        last_value_matched = this_value_matches;
        return TaggedValue(value, ts - lap_start.value());
      },
      source,
      clock_source
    );
  }

  // Pipe factory
  template <typename TDuration, typename TTimePoint, typename FilterFn>
  auto stopwatch(source_fn<TTimePoint> clock_source, FilterFn&& lap_condition)
  -> pipe_fn<TaggedValue<arg_of<FilterFn>, TDuration>, arg_of<FilterFn>> {
    using T = arg_of<FilterFn>;
    return [clock_source, lap_condition = std::forward<FilterFn>(lap_condition)](source_fn<T> source) -> source_fn<TaggedValue<T, TDuration>> {
      return stopwatch<TDuration>(std::move(source), clock_source, std::move(lap_condition));
    };
  }

}