#pragma once

#include <functional>
#include <chrono>
#include <types/core_types.hpp>
#include <types/Range.hpp>
#include <operators/scan.hpp>
#include <operators/map.hpp>
#include <operators/combine.hpp>

namespace rheoscape::operators {

  // ==========================================================================
  // Type helpers
  // ==========================================================================

  // Helper to get interval type from time type
  // For scalars, interval is same as time type
  // For time_points, interval is the duration type
  template <typename TTimePoint>
  struct interval_type {
    using type = TTimePoint;
  };

  template <typename Clock, typename Duration>
  struct interval_type<std::chrono::time_point<Clock, Duration>> {
    using type = Duration;
  };

  template <typename TTimePoint>
  using interval_type_t = typename interval_type<TTimePoint>::type;

  // Helper to get the delta type when subtracting two values
  // For most types, T - T = T, but for QuantityPoint, QP - QP = Quantity
  template <typename T>
  using delta_type_t = decltype(std::declval<T>() - std::declval<T>());

  // ==========================================================================
  // Fully-typed PID structures
  // ==========================================================================

  // PID weights with separate types for each gain
  // Dimensional analysis:
  //   TKp: TCtrl / TProcDelta           (control per unit error)
  //   TKi: TCtrl / TIntegral            (control per unit accumulated error)
  //   TKd: TCtrl / TDerivative          (control per unit error rate)
  template <typename TKp, typename TKi, typename TKd>
  struct PidWeights {
    TKp Kp;
    TKi Ki;
    TKd Kd;
  };

  // Combined input data for PID calculation
  template <typename TProc, typename TTimePoint, typename TKp, typename TKi, typename TKd>
  struct PidData {
    TProc process_variable;
    TProc setpoint;
    TTimePoint timestamp;
    PidWeights<TKp, TKi, TKd> weights;
  };

  // Internal state for PID calculation
  // TProcDelta: error type (setpoint - process_variable)
  // TIntegral: accumulated error type (TProcDelta * TFloatInterval)
  // TCtrl: control output type
  template <typename TCtrl, typename TProcDelta, typename TIntegral, typename TTimePoint>
  struct PidState {
    TCtrl control;
    TProcDelta error;
    TIntegral integral;
    TCtrl p_term;
    TCtrl i_term;
    TCtrl d_term;
    bool is_saturated;
    TTimePoint timestamp;
  };

  // Output type for pid_detailed() - exposes all PID internals
  template <typename TCtrl, typename TProcDelta, typename TIntegral>
  struct PidOutput {
    TCtrl control;       // Final control output
    TCtrl p_term;        // Kp * error
    TCtrl i_term;        // Ki * integral
    TCtrl d_term;        // Kd * derivative
    TProcDelta error;    // setpoint - process_variable
    TIntegral integral;  // Accumulated integral
    bool is_saturated;   // True if control was clamped (anti-windup active)
  };

  // ==========================================================================
  // PID calculator (stays at namespace scope - type aliases shared across functions)
  // ==========================================================================

  // Named callable for PID calculation (scanner function)
  // Full type parameterization for unit-safe arithmetic
  //
  // TIntervalConverter: converts integral-rep interval to float-rep interval
  //   e.g., duration<long, milli> -> duration<float>
  //   or    Quantity<Seconds, int> -> Quantity<Seconds, float>
  //   This preserves the time dimension while enabling float arithmetic.
  template <
    typename TProc,
    typename TTimePoint,
    typename TCtrl,
    typename TKp,
    typename TKi,
    typename TKd,
    typename TIntervalConverter
  >
  struct pid_calculator {
    // Derived types
    using TProcDelta = delta_type_t<TProc>;       // Error type: setpoint - pv
    using TInterval = interval_type_t<TTimePoint>;     // Time delta type (may have integral rep)
    using TFloatInterval = std::invoke_result_t<TIntervalConverter, TInterval>;  // Float-rep interval
    using TIntegral = decltype(std::declval<TProcDelta>() * std::declval<TFloatInterval>());   // Error * time
    using TDerivative = decltype(std::declval<TProcDelta>() / std::declval<TFloatInterval>()); // Error / time

    using StateType = PidState<TCtrl, TProcDelta, TIntegral, TTimePoint>;
    using DataType = PidData<TProc, TTimePoint, TKp, TKi, TKd>;

    std::optional<Range<TCtrl>> clamp_range;
    TIntervalConverter interval_converter;

    RHEOSCAPE_CALLABLE StateType operator()(
      StateType prev_state,
      DataType values
    ) const {
      // Error is delta between desired value and measured value.
      TProcDelta error = values.setpoint - values.process_variable;

      // We don't assume perfectly even timestamps, so we calculate the time delta too.
      TInterval time_delta = values.timestamp - prev_state.timestamp;

      // Convert interval to floating-point representation for arithmetic.
      // This preserves the time dimension (e.g., seconds) while enabling float math.
      TFloatInterval dt = interval_converter(time_delta);

      // Compute integral (accumulated error over time) and derivative (error rate).
      // TIntegral has units like °C·s, TDerivative has units like °C/s
      TIntegral integral = prev_state.integral + error * dt;
      TDerivative derivative = (error - prev_state.error) / dt;

      // Calculate individual PID terms
      // Unit math:
      //   p_term = Kp * error         : (TCtrl/TProcDelta) * TProcDelta = TCtrl
      //   i_term = Ki * integral      : (TCtrl/TIntegral) * TIntegral = TCtrl
      //   d_term = Kd * derivative    : (TCtrl/TDerivative) * TDerivative = TCtrl
      TCtrl p_term = values.weights.Kp * error;
      TCtrl i_term = values.weights.Ki * integral;
      TCtrl d_term = values.weights.Kd * derivative;
      TCtrl control = p_term + i_term + d_term;

      bool is_saturated = false;

      // Clamp the control variable and disable integration
      // when the control variable goes out of range
      // to prevent integrator windup.
      if (clamp_range.has_value()) {
        if (control > clamp_range.value().max) {
          control = clamp_range.value().max;
          integral = prev_state.integral;
          i_term = values.weights.Ki * integral;
          is_saturated = true;
        } else if (control < clamp_range.value().min) {
          control = clamp_range.value().min;
          integral = prev_state.integral;
          i_term = values.weights.Ki * integral;
          is_saturated = true;
        }
      }

      return StateType {
        control,
        error,
        integral,
        p_term,
        i_term,
        d_term,
        is_saturated,
        values.timestamp
      };
    }
  };

  // ==========================================================================
  // Fully-typed PID source function factories
  // ==========================================================================

  // Internal helper: creates the combined and calculated source
  template <
    typename TProc,
    typename TTimePoint,
    typename TCtrl,
    typename TKp,
    typename TKi,
    typename TKd,
    typename TIntervalConverter
  >
  auto pid_calculate(
    source_fn<TProc> process_variable_source,
    source_fn<TProc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TKp, TKi, TKd>> weights_source,
    std::optional<Range<TCtrl>> clamp_range,
    TIntervalConverter&& interval_converter
  ) {
    using Calculator = pid_calculator<TProc, TTimePoint, TCtrl, TKp, TKi, TKd, std::decay_t<TIntervalConverter>>;
    using StateType = typename Calculator::StateType;
    using DataType = typename Calculator::DataType;

    // Named callable for combining PID input sources into PidData.
    struct DataCombiner {
      RHEOSCAPE_CALLABLE DataType operator()(
        TProc process_variable,
        TProc setpoint,
        TTimePoint timestamp,
        PidWeights<TKp, TKi, TKd> weights
      ) const {
        return DataType {
          process_variable,
          setpoint,
          timestamp,
          weights
        };
      }
    };

    source_fn<DataType> combined_source =
      combine(process_variable_source, setpoint_source, clock_source, weights_source)
      | map(DataCombiner{});

    return scan(
      combined_source,
      StateType{},
      Calculator{
        clamp_range,
        std::forward<TIntervalConverter>(interval_converter)
      }
    );
  }

  // Fully-typed PID controller
  // Returns just the control output
  template <
    typename TProc,
    typename TTimePoint,
    typename TCtrl,
    typename TKp,
    typename TKi,
    typename TKd,
    typename TIntervalConverter
  >
  source_fn<TCtrl> pid(
    source_fn<TProc> process_variable_source,
    source_fn<TProc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TKp, TKi, TKd>> weights_source,
    std::optional<Range<TCtrl>> clamp_range,
    TIntervalConverter&& interval_converter
  ) {
    using Calculator = pid_calculator<TProc, TTimePoint, TCtrl, TKp, TKi, TKd, std::decay_t<TIntervalConverter>>;
    using TProcDelta = typename Calculator::TProcDelta;
    using TIntegral = typename Calculator::TIntegral;

    struct ControlExtractor {
      RHEOSCAPE_CALLABLE TCtrl operator()(PidState<TCtrl, TProcDelta, TIntegral, TTimePoint> state) const {
        return state.control;
      }
    };

    auto calculated_source = pid_calculate<TProc, TTimePoint, TCtrl, TKp, TKi, TKd>(
      process_variable_source,
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range,
      std::forward<TIntervalConverter>(interval_converter)
    );

    return map(calculated_source, ControlExtractor{});
  }

  // Fully-typed PID controller with detailed output
  // Returns PidOutput with all internal terms exposed
  template <
    typename TProc,
    typename TTimePoint,
    typename TCtrl,
    typename TKp,
    typename TKi,
    typename TKd,
    typename TIntervalConverter
  >
  auto pid_detailed(
    source_fn<TProc> process_variable_source,
    source_fn<TProc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TKp, TKi, TKd>> weights_source,
    std::optional<Range<TCtrl>> clamp_range,
    TIntervalConverter&& interval_converter
  ) {
    using Calculator = pid_calculator<TProc, TTimePoint, TCtrl, TKp, TKi, TKd, std::decay_t<TIntervalConverter>>;
    using TProcDelta = typename Calculator::TProcDelta;
    using TIntegral = typename Calculator::TIntegral;
    using OutputType = PidOutput<TCtrl, TProcDelta, TIntegral>;

    struct OutputExtractor {
      RHEOSCAPE_CALLABLE OutputType operator()(PidState<TCtrl, TProcDelta, TIntegral, TTimePoint> state) const {
        return OutputType {
          state.control,
          state.p_term,
          state.i_term,
          state.d_term,
          state.error,
          state.integral,
          state.is_saturated
        };
      }
    };

    auto calculated_source = pid_calculate<TProc, TTimePoint, TCtrl, TKp, TKi, TKd>(
      process_variable_source,
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range,
      std::forward<TIntervalConverter>(interval_converter)
    );

    return map(calculated_source, OutputExtractor{});
  }

  // ==========================================================================
  // Simplified scalar-only overloads
  // These use TCalc for all value types, delegating to the fully-typed version
  // ==========================================================================

  // Simplified PID controller for scalar types with interval converter
  template <typename TCalc, typename TTimePoint, typename TIntervalConverter>
  source_fn<TCalc> pid(
    source_fn<TCalc> process_variable_source,
    source_fn<TCalc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source,
    std::optional<Range<TCalc>> clamp_range,
    TIntervalConverter&& interval_converter
  ) {
    return pid<TCalc, TTimePoint, TCalc, TCalc, TCalc, TCalc>(
      process_variable_source,
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range,
      std::forward<TIntervalConverter>(interval_converter)
    );
  }

  // Simplified PID controller for pure scalar types (no converter needed)
  // Assumes TTimePoint - TTimePoint yields something directly usable in arithmetic with TCalc
  template <typename TCalc, typename TTimePoint>
  source_fn<TCalc> pid(
    source_fn<TCalc> process_variable_source,
    source_fn<TCalc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source,
    std::optional<Range<TCalc>> clamp_range = std::nullopt
  ) {
    using TInterval = interval_type_t<TTimePoint>;

    struct IntervalConverter {
      RHEOSCAPE_CALLABLE TCalc operator()(TInterval t) const {
        return static_cast<TCalc>(t);
      }
    };

    return pid<TCalc, TTimePoint>(
      process_variable_source,
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range,
      IntervalConverter{}
    );
  }

  // Simplified pid_detailed for scalar types with interval converter
  template <typename TCalc, typename TTimePoint, typename TIntervalConverter>
  source_fn<PidOutput<TCalc, TCalc, TCalc>> pid_detailed(
    source_fn<TCalc> process_variable_source,
    source_fn<TCalc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source,
    std::optional<Range<TCalc>> clamp_range,
    TIntervalConverter&& interval_converter
  ) {
    return pid_detailed<TCalc, TTimePoint, TCalc, TCalc, TCalc, TCalc>(
      process_variable_source,
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range,
      std::forward<TIntervalConverter>(interval_converter)
    );
  }

  // Simplified pid_detailed for pure scalar types
  template <typename TCalc, typename TTimePoint>
  source_fn<PidOutput<TCalc, TCalc, TCalc>> pid_detailed(
    source_fn<TCalc> process_variable_source,
    source_fn<TCalc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source,
    std::optional<Range<TCalc>> clamp_range = std::nullopt
  ) {
    using TInterval = interval_type_t<TTimePoint>;

    struct IntervalConverter {
      RHEOSCAPE_CALLABLE TCalc operator()(TInterval t) const {
        return static_cast<TCalc>(t);
      }
    };

    return pid_detailed<TCalc, TTimePoint>(
      process_variable_source,
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range,
      IntervalConverter{}
    );
  }

  // ==========================================================================
  // Pipe versions (simplified scalar-only for ergonomics)
  // ==========================================================================

  // TODO: Migrate pipe factories to generic SourceT to work with operator|.
  // Currently they take source_fn<TCalc> explicitly.
  namespace detail {
    template <typename TCalc, typename TTimePoint, typename TIntervalConverter>
    struct PidPipeFactory {
      source_fn<TCalc> setpoint_source;
      source_fn<TTimePoint> clock_source;
      source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source;
      std::optional<Range<TCalc>> clamp_range;
      TIntervalConverter interval_converter;

      RHEOSCAPE_CALLABLE auto operator()(source_fn<TCalc> process_variable_source) const {
        return pid<TCalc, TTimePoint>(
          process_variable_source, setpoint_source, clock_source,
          weights_source, clamp_range, interval_converter
        );
      }
    };

    template <typename TCalc, typename TTimePoint>
    struct PidPipeFactoryNoConverter {
      source_fn<TCalc> setpoint_source;
      source_fn<TTimePoint> clock_source;
      source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source;
      std::optional<Range<TCalc>> clamp_range;

      RHEOSCAPE_CALLABLE auto operator()(source_fn<TCalc> process_variable_source) const {
        return pid<TCalc, TTimePoint>(
          process_variable_source, setpoint_source, clock_source,
          weights_source, clamp_range
        );
      }
    };

    template <typename TCalc, typename TTimePoint, typename TIntervalConverter>
    struct PidDetailedPipeFactory {
      source_fn<TCalc> setpoint_source;
      source_fn<TTimePoint> clock_source;
      source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source;
      std::optional<Range<TCalc>> clamp_range;
      TIntervalConverter interval_converter;

      RHEOSCAPE_CALLABLE auto operator()(source_fn<TCalc> process_variable_source) const {
        return pid_detailed<TCalc, TTimePoint>(
          process_variable_source, setpoint_source, clock_source,
          weights_source, clamp_range, interval_converter
        );
      }
    };

    template <typename TCalc, typename TTimePoint>
    struct PidDetailedPipeFactoryNoConverter {
      source_fn<TCalc> setpoint_source;
      source_fn<TTimePoint> clock_source;
      source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source;
      std::optional<Range<TCalc>> clamp_range;

      RHEOSCAPE_CALLABLE auto operator()(source_fn<TCalc> process_variable_source) const {
        return pid_detailed<TCalc, TTimePoint>(
          process_variable_source, setpoint_source, clock_source,
          weights_source, clamp_range
        );
      }
    };
  }

  template <typename TCalc, typename TTimePoint, typename TIntervalConverter>
  auto pid(
    source_fn<TCalc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source,
    std::optional<Range<TCalc>> clamp_range,
    TIntervalConverter&& interval_converter
  ) {
    return detail::PidPipeFactory<TCalc, TTimePoint, std::decay_t<TIntervalConverter>>{
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range,
      std::forward<TIntervalConverter>(interval_converter)
    };
  }

  template <typename TCalc, typename TTimePoint>
  auto pid(
    source_fn<TCalc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source,
    std::optional<Range<TCalc>> clamp_range = std::nullopt
  ) {
    return detail::PidPipeFactoryNoConverter<TCalc, TTimePoint>{
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range
    };
  }

  template <typename TCalc, typename TTimePoint, typename TIntervalConverter>
  auto pid_detailed(
    source_fn<TCalc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source,
    std::optional<Range<TCalc>> clamp_range,
    TIntervalConverter&& interval_converter
  ) {
    return detail::PidDetailedPipeFactory<TCalc, TTimePoint, std::decay_t<TIntervalConverter>>{
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range,
      std::forward<TIntervalConverter>(interval_converter)
    };
  }

  template <typename TCalc, typename TTimePoint>
  auto pid_detailed(
    source_fn<TCalc> setpoint_source,
    source_fn<TTimePoint> clock_source,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source,
    std::optional<Range<TCalc>> clamp_range = std::nullopt
  ) {
    return detail::PidDetailedPipeFactoryNoConverter<TCalc, TTimePoint>{
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range
    };
  }

}
