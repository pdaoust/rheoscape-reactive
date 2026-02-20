#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>
#include <operators/scan.hpp>
#include <operators/filter_map.hpp>

namespace rheoscape::operators {

  // Like `timestamp`, but resets the clock every time `lap_condition` is met.
  // It will glom contiguous values where `lap_condition` is met,
  // so you could use something like `temperature > 20.0f`
  // and it wouldn't restart the stopwatch at 0 for every value
  // while the temperature was above 20.

  template <typename TDuration, typename SourceT, typename ClockSourceT>
    requires concepts::Source<SourceT> && concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TDuration>
  auto stopwatch_changes(SourceT source, ClockSourceT clock_source) {
    using T = source_value_t<SourceT>;
    using TTimePoint = source_value_t<ClockSourceT>;
    // First timestamp is the lap start; second timestamp is the timestamp of the current value.
    using TAcc = std::tuple<T, TTimePoint, TTimePoint>;

    struct LapScanner {
      RHEOSCAPE_CALLABLE std::optional<TAcc> operator()(std::optional<TAcc> acc, T value, TTimePoint ts) const {
        if (!acc.has_value() || value != std::get<0>(acc.value())) {
          // We've just started the stream or transitioned to a new value,
          // which means we start a new lap.
          return TAcc(value, ts, ts);
        }
        // Remember the lap start but give the current timestamp.
        return TAcc(value, std::get<1>(acc.value()), ts);
      }
    };

    return combine(std::move(source), std::move(clock_source))
      | scan(std::optional<TAcc>{}, LapScanner{})
      | filter_map([](std::optional<TAcc> v) -> std::optional<TaggedValue<T, TDuration>> {
        if (v.has_value()) {
            return TaggedValue(std::get<0>(v.value()), std::get<2>(v.value()) - std::get<1>(v.value()));
        }
        return std::nullopt;
      });
  }

  namespace detail {
    template <typename TDuration, typename ClockSourceT, typename FilterFn>
    struct StopwatchChangesPipeFactory {
      ClockSourceT clock_source;
      FilterFn lap_condition;

      template <typename SourceT>
        requires concepts::Source<SourceT> &&
                 concepts::Predicate<FilterFn, source_value_t<SourceT>>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return stopwatch_changes<TDuration>(std::move(source), ClockSourceT(clock_source), FilterFn(lap_condition));
      }
    };
  }

  // Pipe factory
  template <typename TDuration, typename ClockSourceT, typename FilterFn>
    requires concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TDuration>
  auto stopwatch_changes(ClockSourceT clock_source, FilterFn&& lap_condition) {
    return detail::StopwatchChangesPipeFactory<TDuration, ClockSourceT, std::decay_t<FilterFn>>{
      std::move(clock_source),
      std::forward<FilterFn>(lap_condition)
    };
  }

}
