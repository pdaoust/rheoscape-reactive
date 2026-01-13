#pragma once

#include <functional>
#include <core_types.hpp>
#include <util.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>
#include <operators/scan.hpp>
#include <operators/timestamp.hpp>

namespace rheo::operators {

  // Smooth an input reading over a moving average time interval,
  // using the exponential moving average or single-pole IIR method.
  // If you think of this process as a low-pass filter,
  // the time constant is the period of the cutoff frequency.
  // Any movements slower than this will get integrated;
  // any movements faster than this will get filtered out.
  template <typename TVal, typename TTime, typename TInterval, typename TMapIntervalToRep>
  source_fn<TVal> exponential_moving_average(
    source_fn<TVal> source,
    source_fn<TTime> clock_source,
    source_fn<TInterval> time_constant_source,
    TMapIntervalToRep&& map_interval_to_rep
  ) {
    using TRep = return_of<std::decay_t<TMapIntervalToRep>>;

    return combine(std::make_tuple<TVal, TInterval>, source, time_constant_source)
      | timestamp<std::tuple<TVal, TInterval>>(clock_source)
      | scan(
        std::optional<TaggedValue<TVal, TTime>>{},
        [map_interval_to_rep = std::forward<TMapIntervalToRep>(map_interval_to_rep)]
        (std::optional<TaggedValue<TVal, TTime>> prev, TaggedValue<std::tuple<TVal, TInterval>, TTime> next) {
          TVal next_value = std::get<0>(next.value);

          if (!prev.has_value()) {
            // First run, no average to be taken.
            return std::optional<TaggedValue<TVal, TTime>> { { next_value, next.tag } };
          }

          TVal prev_value = prev.value().value;
          TInterval time_constant = std::get<1>(next.value);
          TInterval time_delta = next.tag - prev.value().tag;
          TRep alpha = (TRep)1.0 - pow((TRep)M_E, -map_interval_to_rep(time_delta) / map_interval_to_rep(time_constant));
          TVal integrated = prev_value + (next_value - prev_value) * alpha;
          return std::optional<TaggedValue<TVal, TTime>> { { integrated, next.tag } };
        }
      )
      | map([](std::optional<TaggedValue<TVal, TTime>> value) { return value.value().value; });
  }

  // A simpler version for use with scalar time rather than std::chrono time.
  template <typename TVal, typename TTime>
  source_fn<TVal> exponential_moving_average(source_fn<TVal> source, source_fn<TTime> clock_source, source_fn<TTime> time_constant_source) {
    return exponential_moving_average(
      source,
      clock_source,
      time_constant_source,
      [](TTime t) { return (TVal)t; }
    );
  }

  template <typename TVal, typename TTime, typename TInterval, typename TMapIntervalToRep>
  pipe_fn<TVal, TVal> exponential_moving_average(source_fn<TTime> clock_source, source_fn<TTime> time_constant_source, TMapIntervalToRep&& map_interval_to_rep) {
    return [clock_source, time_constant_source, map_interval_to_rep = std::forward<TMapIntervalToRep>(map_interval_to_rep)](source_fn<TVal> source) {
      return exponential_moving_average(source, clock_source, time_constant_source, map_interval_to_rep);
    };
  }

  template <typename TVal, typename TTime>
  pipe_fn<TVal, TVal> exponential_moving_average(source_fn<TTime> clock_source, source_fn<TTime> time_constant_source) {
    return exponential_moving_average(clock_source, time_constant_source, [](TTime t) { return (TVal)t; });
  }

  template <typename TRep, typename TInterval>
  TRep map_chrono_to_scalar(TInterval interval) {
    return (TRep)interval.count();
  }

}