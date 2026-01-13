#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/au_all_units_noio.hpp>
#include <types/Range.hpp>
#include <operators/scan.hpp>
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
    TProc process_variable;
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
    source_fn<TProc> process_variable_source,
    source_fn<TProc> setpoint_source,
    source_fn<TTime> clock_source,
    source_fn<PidWeights<TKp, TKi, TKd>> weights_source,
    std::optional<Range<TCtl>> clamp_range = std::nullopt
  ) {
    source_fn<PidData<TProc, TTime, TKp, TKi, TKd>> combined_source = combine(
      [](TProc process_variable, TProc setpoint, TTime timestamp, PidWeights<TKp, TKi, TKd> weights) {
        return PidData<TProc, TTime, TKp, TKi, TKd> { process_variable, setpoint, timestamp, weights };
      },
      process_variable_source,
      setpoint_source,
      clock_source,
      weights_source
    );

    source_fn<PidState<TP, TI, TCtl, TTime>> calculated_source = scan(
      combined_source,
      PidState<TP, TI, TCtl, TTime> { },
      [clamp_range](PidState<TP, TI, TCtl, TTime> prev_state, PidData<TProc, TTime, TKp, TKi, TKd> values) {
        // Error is delta between desired value and measured value.
        // Error = proportional term
        TP error = values.setpoint - values.process_variable;
        TTime time_delta = values.timestamp - prev_state.timestamp;
        TI integral = prev_state.integral + error * time_delta;
        TD derivative = (error - prev_state.error) / time_delta;
        TCtl control = values.weights.Kp * error + values.weights.Ki * integral + values.weights.Kd * derivative;

        // Clamp the control variable and disable integration
        // when the control variable goes out of range
        // to prevent integrator windup.
        if (clamp_range.has_value()) {
          if (control > clamp_range.value().max) {
            control = clamp_range.value().max;
            integral = prev_state.integral;
          } else if (control < clamp_range.value().min) {
            control = clamp_range.value().min;
            integral = prev_state.integral;
          }
        }
        return PidState<TP, TI, TCtl, TTime> { error, integral, control, values.timestamp };
      }
    );

    return map(calculated_source, [](PidState<TP, TI, TCtl, TTime> value) { return value.control; });
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
    source_fn<TProc> setpoint_source,
    source_fn<TTime> clock_source,
    source_fn<PidWeights<TKp, TKi, TKd>> weights_source,
    std::optional<Range<TCtl>> clamp_range = std::nullopt
  ) {
    return [setpoint_source, clock_source, weights_source](source_fn<TProc> process_variable_source) {
      return pid<TCtl, TProc, TTime, TInterval, TKp, TKi, TKd, TP, TI, TD>(process_variable_source, setpoint_source, clock_source, weights_source);
    };
  }

  template <typename TCalc, typename TTime>
  source_fn<TCalc> pid_scalar(
    source_fn<TCalc> process_variable_source,
    source_fn<TCalc> setpoint_source,
    source_fn<TTime> clock_source,
    source_fn<PidWeights<TCalc, TCalc, TCalc>> weights_source,
    std::optional<Range<TCalc>> clamp_range = std::nullopt
  ) {
    return pid<TCalc, TCalc, TTime, TTime, TCalc, TCalc, TCalc, TCalc, TCalc, TCalc>(process_variable_source, setpoint_source, clock_source, weights_source, clamp_range);
  }

  template <typename TTime>
  using au__t_interval = decltype(std::declval<TTime>() - std::declval<TTime>());

  template <typename TProc>
  using au__t_p = decltype(std::declval<TProc>() - std::declval<TProc>());

  template <typename TProc, typename TTime>
  using au__t_i = decltype(std::declval<au__t_p<TProc>>() * std::declval<au__t_interval<TTime>>());

  template <typename TProc, typename TTime>
  using au__t_d = decltype(std::declval<au__t_p<TProc>>() / std::declval<au__t_interval<TTime>>());

  template <typename TCtl, typename TProc>
  using au__t_kkp = decltype(std::declval<TCtl>() / std::declval<au__t_p<TProc>>());

  template <typename TCtl, typename TProc, typename TTime>
  using au__t_kki = decltype(std::declval<TCtl>() / std::declval<au__t_i<TProc, TTime>>());

  template <typename TCtl, typename TProc, typename TTime>
  using au__t_kd = decltype(std::declval<TCtl>() / std::declval<au__t_d<TProc, TTime>>());

  template <typename TCtl, typename TProc, typename TTime>
  source_fn<TCtl> pid_au(
    source_fn<TProc> process_variable_source,
    source_fn<TProc> setpoint_source,
    source_fn<TTime> clock_source,
    source_fn<PidWeights<au__t_kp<TCtl, TProc>, au__t_ki<TCtl, TProc, TTime>, au__t_kd<TCtl, TProc, TTime>>> weights_source,
    std::optional<Range<TCtl>> clamp_range = std::nullopt
  ) {
    return pid<
      TCtl,
      TProc,
      TTime,
      au__t_interval<TTime>,
      au__t_kp<TCtl, TProc>,
      au__t_ki<TCtl, TProc, TTime>,
      au__t_kd<TCtl, TProc, TTime>,
      au__t_p<TProc>,
      au__t_i<TProc, TTime>,
      au__t_d<TProc, TTime>
    >(
      process_variable_source,
      setpoint_source,
      clock_source,
      weights_source,
      clamp_range
    );
  }

}