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
  template <typename TVal, typename TTimePoint, typename TInterval, typename TIntervalConverter>
  source_fn<TVal> exponential_moving_average(
    source_fn<TVal> source,
    source_fn<TTimePoint> clock_source,
    source_fn<TInterval> time_constant_source,
    TIntervalConverter&& interval_converter
  ) {
    using TIntervalConverterDecayed = std::decay_t<TIntervalConverter>;
    using TFloatInterval = std::invoke_result_t<TIntervalConverterDecayed, TInterval>;

    // Named callable for EMA scan operation.
    struct Scanner {
      TIntervalConverterDecayed interval_converter;

      RHEO_CALLABLE std::optional<TaggedValue<TVal, TTimePoint>> operator()(
        std::optional<TaggedValue<TVal, TTimePoint>> prev,
        TaggedValue<std::tuple<TVal, TInterval>, TTimePoint> next
      ) const {
        TVal next_value = std::get<0>(next.value);

        if (!prev.has_value()) {
          // First run, no average to be taken.
          return std::optional<TaggedValue<TVal, TTimePoint>>{ { next_value, next.tag } };
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
        return std::optional<TaggedValue<TVal, TTimePoint>>{ { integrated, next.tag } };
      }
    };

    // Named callable for extracting value from optional TaggedValue.
    struct ValueExtractor {
      RHEO_CALLABLE TVal operator()(std::optional<TaggedValue<TVal, TTimePoint>> value) const {
        return value.value().value;
      }
    };

    return combine(source, time_constant_source)
      | timestamp<std::tuple<TVal, TInterval>>(clock_source)
      | scan(
        std::optional<TaggedValue<TVal, TTimePoint>>{},
        Scanner{ std::forward<TIntervalConverter>(interval_converter) }
      )
      | map(ValueExtractor{});
  }

  // A simpler version for use with scalar time rather than std::chrono time.
  // Assumes TTimePoint - TTimePoint yields something directly castable to TVal for arithmetic.
  template <typename TVal, typename TTimePoint>
  source_fn<TVal> exponential_moving_average(
    source_fn<TVal> source,
    source_fn<TTimePoint> clock_source,
    source_fn<TTimePoint> time_constant_source
  ) {
    struct IntervalConverter {
      RHEO_CALLABLE TVal operator()(TTimePoint t) const {
        return static_cast<TVal>(t);
      }
    };

    return exponential_moving_average(
      source,
      clock_source,
      time_constant_source,
      IntervalConverter{}
    );
  }

  // Pipe version with interval converter
  template <typename TVal, typename TTimePoint, typename TInterval, typename TIntervalConverter>
  pipe_fn<TVal, TVal> exponential_moving_average(
    source_fn<TTimePoint> clock_source,
    source_fn<TInterval> time_constant_source,
    TIntervalConverter&& interval_converter
  ) {
    using TIntervalConverterDecayed = std::decay_t<TIntervalConverter>;

    struct PipeFactory {
      source_fn<TTimePoint> clock_source;
      source_fn<TInterval> time_constant_source;
      TIntervalConverterDecayed interval_converter;

      RHEO_CALLABLE source_fn<TVal> operator()(source_fn<TVal> source) const {
        return exponential_moving_average(source, clock_source, time_constant_source, interval_converter);
      }
    };

    return PipeFactory{
      clock_source,
      time_constant_source,
      std::forward<TIntervalConverter>(interval_converter)
    };
  }

  // Pipe version for scalar time
  template <typename TVal, typename TTimePoint>
  pipe_fn<TVal, TVal> exponential_moving_average(
    source_fn<TTimePoint> clock_source,
    source_fn<TTimePoint> time_constant_source
  ) {
    struct PipeFactory {
      source_fn<TTimePoint> clock_source;
      source_fn<TTimePoint> time_constant_source;

      RHEO_CALLABLE source_fn<TVal> operator()(source_fn<TVal> source) const {
        struct IntervalConverter {
          RHEO_CALLABLE TVal operator()(TTimePoint t) const {
            return static_cast<TVal>(t);
          }
        };

        return exponential_moving_average(
          source,
          clock_source,
          time_constant_source,
          IntervalConverter{}
        );
      }
    };

    return PipeFactory{clock_source, time_constant_source};
  }

}
