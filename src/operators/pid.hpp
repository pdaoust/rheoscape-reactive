#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/au_all_units_noio.hpp>
#include <types/Range.hpp>
#include <operators/fold.hpp>
#include <operators/map.hpp>
#include <operators/combine.hpp>

namespace rheo::operators {

  template <typename TKp, typename TKi, typename TKd>
  struct PidWeights {
    TKp Kp;
    TKi Ki;
    TKd Kd;
  };

  template <typename TProc, typename TTime, typename TKp, typename TKi, typename TKd>
  struct PidData {
    TProc processVariable;
    TProc setpoint;
    TTime timestamp;
    PidWeights<TKp, TKi, TKd> weights;
  };

  template <typename TP, typename TI, typename TCtl, typename TTime>
  struct PidState {
    TP error;
    TI integral;
    TCtl control;
    TTime timestamp;
  };

  template <
    // The type of the control variable -- that is, the output.
    // Should be either a scalar or a unit value
    // that is the same across TKp * Kp, TKi * Ki, and TKd * Kd.
    typename TCtl,
    // The type of the process variable.
    // If a unit value, it can be either a Quantity or a QuantityPoint.
    typename TProc,
    // The type of the timestamp.
    // If using chrono, it should be a std::chrono::time_point.
    typename TTime,
    // The type of one TTime - another TTime.
    // If using chrono, it should be a std::chrono::duration
    // that matches the duration of TTime's clock type param.
    typename TInterval,
    // The type of the Kp weight, or proportional coefficient.
    // If a unit value, it should be TCtl / TP.
    typename TKp,
    // The type of the Ki weight, or integral coefficient.
    // If a unit value, it should be TCtl / TI.
    typename TKi,
    // The type of the Kd weight, or derivative coefficient.
    // If a unit value, it should be TCtl / TD.
    typename TKd,
    // The type of the error or proportional term.
    // If a unit value, it should be TProc - TProc,
    // so if TProc is a QuantityPoint, TP will be a Quantity.
    typename TP,
    // The type of the integral term.
    // If a unit value, it should be TP * TInterval.
    // This is like saying the accumulated error per time interval.
    // Calculus, baby!
    typename TI,
    // The type of the derivative term.
    // If a unit value, it should be TP / TInterval.
    // This is like saying the instantaneous rate of change of the proportional term.
    typename TD
  >
  source_fn<TCtl> pid(
    source_fn<TProc> processVariableSource,
    source_fn<TProc> setpointSource,
    source_fn<TTime> clockSource,
    source_fn<PidWeights<TKp, TKi, TKd>> weightsSource,
    std::optional<Range<TCtl>> clampRange = std::nullopt
  ) {
    source_fn<PidData<TProc, TTime, TKp, TKi, TKd>> combinedSource = combine(
      processVariableSource,
      setpointSource,
      clockSource,
      weightsSource,
      (combine4_fn<PidData<TProc, TTime, TKp, TKi, TKd>, TProc, TProc, TTime, PidWeights<TKp, TKi, TKd>>)[](TProc processVariable, TProc setpoint, TTime timestamp, PidWeights<TKp, TKi, TKd> weights) {
        return PidData<TProc, TTime, TKp, TKi, TKd> { processVariable, setpoint, timestamp, weights };
      }
    );

    source_fn<PidState<TP, TI, TCtl, TTime>> calculatedSource = fold(
      combinedSource,
      PidState<TP, TI, TCtl, TTime> { },
      (fold_fn<PidState<TP, TI, TCtl, TTime>, PidData<TProc, TTime, TKp, TKi, TKd>>)[clampRange](PidState<TP, TI, TCtl, TTime> prevState, PidData<TProc, TTime, TKp, TKi, TKd> values) {
        // Error is delta between desired value and measured value.
        // Error = proportional term
        TP error = values.setpoint - values.processVariable;
        TTime timeDelta = values.timestamp - prevState.timestamp;
        TI integral = prevState.integral + error * timeDelta;
        TD derivative = (error - prevState.error) / timeDelta;
        TCtl control = values.weights.Kp * error + values.weights.Ki * integral + values.weights.Kd * derivative;

        // Clamp the control variable and disable integration
        // when the control variable goes out of range
        // to prevent integrator windup.
        if (clampRange.has_value()) {
          if (control > clampRange.value().max) {
            control = clampRange.value().max;
            integral = prevState.integral;
          } else if (control < clampRange.value().min) {
            control = clampRange.value().min;
            integral = prevState.integral;
          }
        }
        return PidState<TP, TI, TCtl, TTime> { error, integral, control, values.timestamp };
      }
    );

    return map(calculatedSource, (map_fn<TCtl, PidState<TP, TI, TCtl, TTime>>)[](PidState<TP, TI, TCtl, TTime> value) { return value.control; });
  }

  template <
    typename TCtl,
    typename TProc,
    typename TTime,
    typename TInterval,
    typename TKp,
    typename TKi,
    typename TKd,
    typename TP,
    typename TI,
    typename TD
  >
  pipe_fn<TCtl, TProc> pid(
    source_fn<TProc> setpointSource,
    source_fn<TTime> clockSource,
    source_fn<PidWeights<TKp, TKi, TKd>> weightsSource,
    std::optional<Range<TCtl>> clampRange = std::nullopt
  ) {
    return [setpointSource, clockSource, weightsSource](source_fn<TProc> processVariableSource) {
      return pid<TCtl, TProc, TTime, TInterval, TKp, TKi, TKd, TP, TI, TD>(processVariableSource, setpointSource, clockSource, weightsSource);
    };
  }

  template <typename TCalc, typename TTime>
  source_fn<TCalc> pid_scalar(
    source_fn<TCalc> processVariableSource,
    source_fn<TCalc> setpointSource,
    source_fn<TTime> clockSource,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weightsSource,
    std::optional<Range<TCalc>> clampRange = std::nullopt
  ) {
    return pid<TCalc, TCalc, TTime, TTime, TCalc, TCalc, TCalc, TCalc, TCalc, TCalc>(processVariableSource, setpointSource, clockSource, weightsSource, clampRange);
  }

  template <typename TTime>
  using au_TInterval = decltype(std::declval<TTime>() - std::declval<TTime>());

  template <typename TProc>
  using au_TP = decltype(std::declval<TProc>() - std::declval<TProc>());

  template <typename TProc, typename TTime>
  using au_TI = decltype(std::declval<au_TP<TProc>>() * std::declval<au_TInterval<TTime>>());

  template <typename TProc, typename TTime>
  using au_TD = decltype(std::declval<au_TP<TProc>>() / std::declval<au_TInterval<TTime>>());

  template <typename TCtl, typename TProc>
  using au_TKp = decltype(std::declval<TCtl>() / std::declval<au_TP<TProc>>());

  template <typename TCtl, typename TProc, typename TTime>
  using au_TKi = decltype(std::declval<TCtl>() / std::declval<au_TI<TProc, TTime>>());

  template <typename TCtl, typename TProc, typename TTime>
  using au_TKd = decltype(std::declval<TCtl>() / std::declval<au_TD<TProc, TTime>>());

  template <typename TCtl, typename TProc, typename TTime>
  source_fn<TCtl> pid_au(
    source_fn<TProc> processVariableSource,
    source_fn<TProc> setpointSource,
    source_fn<TTime> clockSource,
    source_fn<PidWeights<au_TKp<TCtl, TProc>, au_TKi<TCtl, TProc, TTime>, au_TKd<TCtl, TProc, TTime>>> weightsSource,
    std::optional<Range<TCtl>> clampRange = std::nullopt
  ) {
    return pid<
      TCtl,
      TProc,
      TTime,
      au_TInterval<TTime>,
      au_TKp<TCtl, TProc>,
      au_TKi<TCtl, TProc, TTime>,
      au_TKd<TCtl, TProc, TTime>,
      au_TP<TProc>,
      au_TI<TProc, TTime>,
      au_TD<TProc, TTime>
    >(
      processVariableSource,
      setpointSource,
      clockSource,
      weightsSource,
      clampRange
    );
  }

}