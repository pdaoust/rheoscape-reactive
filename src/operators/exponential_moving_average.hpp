#pragma once

#include <functional>
#include <cmath>
#include <types/core_types.hpp>
#include <util/misc.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>
#include <operators/scan.hpp>
#include <operators/timestamp.hpp>

namespace rheoscape::operators {

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
  template <typename SourceT, typename ClockSourceT, typename TimeConstantSourceT, typename TIntervalConverter>
    requires concepts::Source<SourceT> && concepts::Source<ClockSourceT> && concepts::Source<TimeConstantSourceT>
  auto exponential_moving_average(
    SourceT source,
    ClockSourceT clock_source,
    TimeConstantSourceT time_constant_source,
    TIntervalConverter&& interval_converter
  ) {
    using TVal = source_value_t<SourceT>;
    using TTimePoint = source_value_t<ClockSourceT>;
    using TInterval = source_value_t<TimeConstantSourceT>;
    using TIntervalConverterDecayed = std::decay_t<TIntervalConverter>;
    using TFloatInterval = std::invoke_result_t<TIntervalConverterDecayed, TInterval>;

    // Named callable for EMA scan operation.
    struct Scanner {
      TIntervalConverterDecayed interval_converter;

      RHEOSCAPE_CALLABLE std::optional<std::tuple<TVal, TTimePoint>> operator()(
        std::optional<std::tuple<TVal, TTimePoint>> prev,
        std::tuple<std::tuple<TVal, TInterval>, TTimePoint> next
      ) const {
        auto [next_v, next_ts] = next;
        auto [next_value, time_constant] = next_v;

        if (!prev.has_value()) {
          // First run, no average to be taken.
          return std::optional<std::tuple<TVal, TTimePoint>>{ { next_value, next_ts } };
        }

        auto [prev_value, prev_ts] = prev.value();
        TInterval time_delta = next_ts - prev_ts;

        // Convert intervals to float-rep for arithmetic.
        // The division time_delta/time_constant is dimensionless (units cancel).
        TFloatInterval dt = interval_converter(time_delta);
        TFloatInterval tau = interval_converter(time_constant);

        // alpha = 1 - e^(-dt/tau)
        // dt/tau is dimensionless, so this works with unit-safe types
        auto ratio = dt / tau;
        auto alpha = decltype(ratio){1} - std::exp(-ratio);

        TVal integrated = prev_value + (next_value - prev_value) * alpha;
        return std::optional<std::tuple<TVal, TTimePoint>>{ { integrated, next_ts } };
      }
    };

    // Named callable for extracting value from optional std::tuple.
    struct ValueExtractor {
      RHEOSCAPE_CALLABLE TVal operator()(std::optional<std::tuple<TVal, TTimePoint>> value) const {
        return std::get<0>(value.value());
      }
    };

    return combine(std::move(source), std::move(time_constant_source))
      | timestamp(std::move(clock_source))
      | scan(
        std::optional<std::tuple<TVal, TTimePoint>>{},
        Scanner{ std::forward<TIntervalConverter>(interval_converter) }
      )
      | map(ValueExtractor{});
  }

  // A simpler version for use with scalar time rather than std::chrono time.
  // Assumes TTimePoint - TTimePoint yields something directly castable to TVal for arithmetic.
  template <typename SourceT, typename ClockSourceT, typename TimeConstantSourceT>
    requires concepts::Source<SourceT> && concepts::Source<ClockSourceT> && concepts::Source<TimeConstantSourceT> &&
             std::is_same_v<source_value_t<ClockSourceT>, source_value_t<TimeConstantSourceT>>
  auto exponential_moving_average(
    SourceT source,
    ClockSourceT clock_source,
    TimeConstantSourceT time_constant_source
  ) {
    using TVal = source_value_t<SourceT>;
    using TTimePoint = source_value_t<ClockSourceT>;

    struct IntervalConverter {
      RHEOSCAPE_CALLABLE TVal operator()(TTimePoint t) const {
        return static_cast<TVal>(t);
      }
    };

    return exponential_moving_average(
      std::move(source),
      std::move(clock_source),
      std::move(time_constant_source),
      IntervalConverter{}
    );
  }

  namespace detail {
    template <typename ClockSourceT, typename TimeConstantSourceT, typename TIntervalConverter>
    struct EmaPipeFactory {
      ClockSourceT clock_source;
      TimeConstantSourceT time_constant_source;
      TIntervalConverter interval_converter;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return exponential_moving_average(
          std::move(source),
          ClockSourceT(clock_source),
          TimeConstantSourceT(time_constant_source),
          TIntervalConverter(interval_converter)
        );
      }
    };

    template <typename TVal, typename ClockSourceT, typename TimeConstantSourceT>
    struct EmaPipeFactoryScalar {
      ClockSourceT clock_source;
      TimeConstantSourceT time_constant_source;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        using TTimePoint = source_value_t<ClockSourceT>;

        struct IntervalConverter {
          RHEOSCAPE_CALLABLE TVal operator()(TTimePoint t) const {
            return static_cast<TVal>(t);
          }
        };

        return exponential_moving_average(
          std::move(source),
          ClockSourceT(clock_source),
          TimeConstantSourceT(time_constant_source),
          IntervalConverter{}
        );
      }
    };
  }

  // Pipe version with interval converter
  template <typename ClockSourceT, typename TimeConstantSourceT, typename TIntervalConverter>
    requires concepts::Source<ClockSourceT> && concepts::Source<TimeConstantSourceT> &&
             (!concepts::Source<std::decay_t<TIntervalConverter>>)
  auto exponential_moving_average(
    ClockSourceT clock_source,
    TimeConstantSourceT time_constant_source,
    TIntervalConverter&& interval_converter
  ) {
    return detail::EmaPipeFactory<ClockSourceT, TimeConstantSourceT, std::decay_t<TIntervalConverter>>{
      std::move(clock_source),
      std::move(time_constant_source),
      std::forward<TIntervalConverter>(interval_converter)
    };
  }

  // Pipe version for scalar time
  template <typename TVal, typename ClockSourceT, typename TimeConstantSourceT>
    requires concepts::Source<ClockSourceT> && concepts::Source<TimeConstantSourceT> &&
             std::is_same_v<source_value_t<ClockSourceT>, source_value_t<TimeConstantSourceT>>
  auto exponential_moving_average(
    ClockSourceT clock_source,
    TimeConstantSourceT time_constant_source
  ) {
    return detail::EmaPipeFactoryScalar<TVal, ClockSourceT, TimeConstantSourceT>{
      std::move(clock_source), std::move(time_constant_source)
    };
  }

}
