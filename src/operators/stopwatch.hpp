#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>
#include <operators/map_tuple.hpp>

namespace rheo::operators {

  // Like `timestamp`, but resets the clock every time `lap_condition` is met.
  // It will glom contiguous values where `lap_condition` is met,
  // so you could use something like `temperature > 20.0f`
  // and it wouldn't restart the stopwatch at 0 for every value
  // while the temperature was above 20.

  template <typename TDuration, typename T, typename TTimePoint, typename FilterFn>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TDuration>
  source_fn<TaggedValue<T, TDuration>> stopwatch(source_fn<T> source, source_fn<TTimePoint> clock_source, FilterFn&& lap_condition) {
    using FilterFnDecayed = std::decay_t<FilterFn>;

    // This is kinda sneaky. We're turning `combine` into a reducer here,
    // because the mapping callback has state.
    struct LapMapper {
      FilterFnDecayed lap_condition;
      mutable std::optional<TTimePoint> lap_start;
      mutable bool last_value_matched = false;

      RHEO_CALLABLE TaggedValue<T, TDuration> operator()(T value, TTimePoint ts) const {
        bool this_value_matches = lap_condition(value);
        if (!lap_start.has_value() || (!last_value_matched && this_value_matches)) {
          // We've either just started the stream
          // or transitioned from a no-match state to a match state.
          lap_start = ts;
        }

        last_value_matched = this_value_matches;
        return TaggedValue(value, ts - lap_start.value());
      }
    };

    return combine(source, clock_source)
      | map_tuple(LapMapper{ std::forward<FilterFn>(lap_condition) });
  }

  namespace detail {
    template <typename TDuration, typename TTimePoint, typename FilterFn>
    struct StopwatchPipeFactory {
      source_fn<TTimePoint> clock_source;
      FilterFn lap_condition;

      template <typename T>
        requires concepts::Predicate<FilterFn, T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return stopwatch<TDuration>(std::move(source), clock_source, FilterFn(lap_condition));
      }
    };
  }

  // Pipe factory
  template <typename TDuration, typename TTimePoint, typename FilterFn>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TDuration>
  auto stopwatch(source_fn<TTimePoint> clock_source, FilterFn&& lap_condition) {
    return detail::StopwatchPipeFactory<TDuration, TTimePoint, std::decay_t<FilterFn>>{
      clock_source,
      std::forward<FilterFn>(lap_condition)
    };
  }

}
