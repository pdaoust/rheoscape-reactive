#ifndef RHEOSCAPE_PID
#define RHEOSCAPE_PID

#include <functional>
#include <core_types.hpp>
#include <types/au_noio.hpp>
#include <operators/fold.hpp>
#include <operators/map.hpp>
#include <operators/zip.hpp>

template <typename TVal>
struct PidWeights {
  TVal Kp;
  TVal Ki;
  TVal Kd;
  PidWeights(TVal Kp, TVal Ki, TVal Kd)
  : Kp(Kp), Ki(Ki), Kd(Kd)
  { }
};

template <typename TVal, typename TTime>
struct PidData {
  TVal processVariable;
  TVal setpoint;
  TTime timestamp;
  PidWeights<TVal> weights;
};

template <typename TVal, typename TTime>
struct PidState {
  TVal error;
  TVal integral;
  TVal control;
  TTime timestamp;
};

template <typename TVal, typename TTime>
source_fn<TVal> pid_(
  source_fn<TVal> processVariableSource,
  source_fn<TVal> setpointSource,
  source_fn<TTime> clockSource,
  source_fn<PidWeights<TVal>> weightsSource
) {
  source_fn<PidData<TVal, TTime>> zippedSource = zip_<PidData<TVal, TTime>, TVal, TVal, TTime, PidWeights<TVal>>(
    processVariableSource,
    setpointSource,
    clockSource,
    weightsSource,
    [](TVal processVariable, TVal setpoint, TTime timestamp, PidWeights<TVal> weights) {
      return PidData<TVal, TTime> { processVariable, setpoint, timestamp, weights };
    }
  );

  source_fn<PidState<TVal, TTime>> calculatedSource = fold_<PidState<TVal, TTime>, PidData<TVal, TTime>>(
    zippedSource,
    PidState<TVal, TTime> { 0, 0, 0, 0 },
    [](PidState<TVal, TTime> prevState, PidData<TVal, TTime> values) {
      // Error is delta between desired value and measured value.
      // Error = proportional term
      TVal error = values.setpoint - values.processVariable;
      TTime timeDelta = values.timestamp - prevState.timestamp;
      TVal integral = prevState.integral + error * timeDelta;
      TVal derivative = (error - prevState.error) / timeDelta;
      TVal control = values.weights.Kp * error + values.weights.Ki * integral + values.weights.Kd * derivative;
      return PidState<TVal, TTime> { error, integral, control, values.timestamp };
    }
  );

  return map_<TVal, PidState<TVal, TTime>>(calculatedSource, [](PidState<TVal, TTime> value) { return value.control; });
}

template <typename TVal, typename TTime>
pipe_fn<TVal, TVal> pid(source_fn<TVal> setpointSource, source_fn<TTime> clockSource, source_fn<PidWeights<TVal>> weightsSource) {
  return [setpointSource, clockSource, weightsSource](source_fn<TVal> processVariableSource) {
    return pid_(processVariableSource, setpointSource, clockSource, weightsSource);
  };
}

#endif