#pragma once

#include <functional>
#include <cmath>
#include <core_types.hpp>
#include <util.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>
#include <operators/scan.hpp>
#include <operators/timestamp.hpp>

namespace rheo::operators {

  // Named callable for EMA scan operation
  // TIntervalConverter: converts integral-rep interval to float-rep interval
  //   e.g., duration<long, milli> -> duration<float>
  //   This preserves the time dimension while enabling float arithmetic.
  //   The ratio time_delta/time_constant is dimensionless since units cancel.
  template <typename TVal, typename TTime, typename TInterval, typename TIntervalConverter>
  struct ema_scanner {
    using TFloatInterval = std::invoke_result_t<TIntervalConverter, TInterval>;

    TIntervalConverter interval_converter;

    RHEO_NOINLINE std::optional<TaggedValue<TVal, TTime>> operator()(
      std::optional<TaggedValue<TVal, TTime>> prev,
      TaggedValue<std::tuple<TVal, TInterval>, TTime> next
    ) const {
      TVal next_value = std::get<0>(next.value);

      if (!prev.has_value()) {
        // First run, no average to be taken.
        return std::optional<TaggedValue<TVal, TTime>>{ { next_value, next.tag } };
      }

      TVal prev_value = prev.value().value;
      TInterval time_constant = std::get<1>(next.value);
      TInterval time_delta = next.tag - prev.value().tag;

      // Convert intervals to float-rep for arithmetic.
      // The division time_delta/time_constant is dimensionless (units cancel).
      TFloatInterval dt = interval_converter(time_delta);
      TFloatInterval tau = interval_converter(time_constant);

      // alpha = 1 - e^(-dt/tau)
      // dt/tau is dimensionless, so this works with unit-safe types
      auto ratio = dt / tau;
      auto alpha = decltype(ratio){1} - std::exp(-ratio);

      TVal integrated = prev_value + (next_value - prev_value) * alpha;
      return std::optional<TaggedValue<TVal, TTime>>{ { integrated, next.tag } };
    }
  };

  // Named callable for extracting value from optional TaggedValue
  template <typename TVal, typename TTime>
  struct ema_value_extractor {
    RHEO_NOINLINE TVal operator()(std::optional<TaggedValue<TVal, TTime>> value) const {
      return value.value().value;
    }
  };

  // Smooth an input reading over a moving average time interval,
  // using the exponential moving average or single-pole IIR method.
  // If you think of this process as a low-pass filter,
  // the time constant is the period of the cutoff frequency.
  // Any movements slower than this will get integrated;
  // any movements faster than this will get filtered out.
  //
  // TIntervalConverter: converts integral-rep interval to float-rep interval
  //   e.g., duration<long, milli> -> duration<float>
  //   The ratio time_delta/time_constant is dimensionless since units cancel.
  template <typename TVal, typename TTime, typename TInterval, typename TIntervalConverter>
  source_fn<TVal> exponential_moving_average(
    source_fn<TVal> source,
    source_fn<TTime> clock_source,
    source_fn<TInterval> time_constant_source,
    TIntervalConverter&& interval_converter
  ) {
    return combine(std::make_tuple<TVal, TInterval>, source, time_constant_source)
      | timestamp<std::tuple<TVal, TInterval>>(clock_source)
      | scan(
        std::optional<TaggedValue<TVal, TTime>>{},
        ema_scanner<TVal, TTime, TInterval, std::decay_t<TIntervalConverter>>{
          std::forward<TIntervalConverter>(interval_converter)
        }
      )
      | map(ema_value_extractor<TVal, TTime>{});
  }

  // A simpler version for use with scalar time rather than std::chrono time.
  // Assumes TTime - TTime yields something directly castable to TVal for arithmetic.
  template <typename TVal, typename TTime>
  source_fn<TVal> exponential_moving_average(
    source_fn<TVal> source,
    source_fn<TTime> clock_source,
    source_fn<TTime> time_constant_source
  ) {
    return exponential_moving_average(
      source,
      clock_source,
      time_constant_source,
      [](TTime t) { return static_cast<TVal>(t); }
    );
  }

  // Pipe version with interval converter
  template <typename TVal, typename TTime, typename TInterval, typename TIntervalConverter>
  pipe_fn<TVal, TVal> exponential_moving_average(
    source_fn<TTime> clock_source,
    source_fn<TInterval> time_constant_source,
    TIntervalConverter&& interval_converter
  ) {
    return [
      clock_source,
      time_constant_source,
      interval_converter = std::forward<TIntervalConverter>(interval_converter)
    ](source_fn<TVal> source) {
      return exponential_moving_average(source, clock_source, time_constant_source, interval_converter);
    };
  }

  // Pipe version for scalar time
  template <typename TVal, typename TTime>
  pipe_fn<TVal, TVal> exponential_moving_average(
    source_fn<TTime> clock_source,
    source_fn<TTime> time_constant_source
  ) {
    return [clock_source, time_constant_source](source_fn<TVal> source) {
      return exponential_moving_average(
        source,
        clock_source,
        time_constant_source,
        [](TTime t) { return static_cast<TVal>(t); }
      );
    };
  }

}
