#ifndef RHEOSCAPE_PID
#define RHEOSCAPE_PID

#include <functional>
#include <core_types.hpp>
#include <operators/fold.hpp>
#include <operators/zip.hpp>

template <typename T>
struct PidWeights {
  T Kp;
  T Ki;
  T Kd;
  PidWeights(T Kp, T Ki, T Kd)
  : Kp(Kp), Ki(Ki), Kd(Kd)
  { }
};

template <typename TProc, typename TTime>
struct PidData {
  TProc processVariable;
  TProc setpoint;
  TTime timestamp;
  PidWeights<TProc> weights;
};

template <typename TError, typename TIntegral, typename TControl, typename TTime>
struct PidState {
  TError error;
  TIntegral integral;
  TControl control;
  TTime timestamp;
};

template <typename TProc, typename TError, typename TIntegral, typename TControl, typename TTime, typename TInterval>
source_fn<TControl> pid_(source_fn<TProc> processVariableSource, source_fn<TProc> setpointSource, source_fn<TTime> clockSource, source_fn<PidWeights<TProc>> weightsSource) {
  source_fn<PidData<TProc, TTime>> zippedSource = zip_(
    processVariableSource,
    setpointSource,
    clockSource,
    weightsSource,
    [](TProc processVariable, TProc setpoint, TTime timestamp, PidWeights<TProc> weights) {
      return PidData { processVariable, setpoint, timestamp, weights };
    }
  );

  source_fn<PidState<typename TError, typename TIntegral, typename TControl, typename TTime>> calculatedSource = fold_(
    zippedSource,
    PidState { 0, 0, 0, 0 },
    [](pidState prevState, zipped values) {
      // Error is delta between desired value and measured value.
      // Error = proportional term
      TError error = values.setpoint - values.processVariable;
      TInterval timeDelta = values.timestamp - prevState.timestamp;
      TIntegral integral = prevState.integral + error * timeDelta;
      auto derivative = (error - prevState.error) / timeDelta;
      TControl control = values.Kp * error + values.Ki * integral + values.Kd * derivative;
      return PidState { error, integral, control, values.timestamp };
    }
  );

  return map_(calculatedSource, [](pidState value) { return value.control; });
}

template <typename TProc, typename TError, typename TIntegral, typename TControl, typename TTime, typename TInterval>
pipe_fn<TProc, TControl> pid(source_fn<TProc> setpointSource, source_fn<TTime> clockSource, source_fn<PidWeights<TProc>> weightsSource) {
  return [setpointSource, clockSource, weightsSource](source_fn<T> processVariableSource) {
    return pid_(processVariableSource, setpointSource, clockSource, weightsSource);
  };
}

#endif