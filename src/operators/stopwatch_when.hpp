#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>

namespace rheoscape::operators {

  // Like `timestamp`, but resets the clock every time `lap_condition` is met.
  // It will glom contiguous values where `lap_condition` is met,
  // so you could use something like `temperature > 20.0f`
  // and it wouldn't restart the stopwatch at 0 for every value
  // while the temperature was above 20.

  template <typename TDuration, typename SourceT, typename ClockSourceT, typename FilterFn>
    requires concepts::Source<SourceT> && concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TDuration> &&
             concepts::Predicate<FilterFn, source_value_t<SourceT>>
  auto stopwatch_when(SourceT source, ClockSourceT clock_source, FilterFn&& lap_condition) {
    using T = source_value_t<SourceT>;
    using TTimePoint = source_value_t<ClockSourceT>;
    using FilterFnDecayed = std::decay_t<FilterFn>;

    // This is kinda sneaky. We're turning `combine` into a reducer here,
    // because the mapping callback has state.
    struct LapMapper {
      FilterFnDecayed lap_condition;
      mutable std::optional<TTimePoint> lap_start;
      mutable bool last_value_matched = false;

      RHEOSCAPE_CALLABLE TaggedValue<T, TDuration> operator()(T value, TTimePoint ts) const {
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

    return combine(std::move(source), std::move(clock_source))
      | map(LapMapper{ std::forward<FilterFn>(lap_condition) });
  }

  namespace detail {
    template <typename TDuration, typename ClockSourceT, typename FilterFn>
    struct StopwatchWhenPipeFactory {
      ClockSourceT clock_source;
      FilterFn lap_condition;

      template <typename SourceT>
        requires concepts::Source<SourceT> &&
                 concepts::Predicate<FilterFn, source_value_t<SourceT>>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return stopwatch_when<TDuration>(std::move(source), ClockSourceT(clock_source), FilterFn(lap_condition));
      }
    };
  }

  // Pipe factory
  template <typename TDuration, typename ClockSourceT, typename FilterFn>
    requires concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TDuration>
  auto stopwatch_when(ClockSourceT clock_source, FilterFn&& lap_condition) {
    return detail::StopwatchWhenPipeFactory<TDuration, ClockSourceT, std::decay_t<FilterFn>>{
      std::move(clock_source),
      std::forward<FilterFn>(lap_condition)
    };
  }

}
