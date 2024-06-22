#ifndef RHEOSCAPE_EXPONENTIAL_MOVING_AVERAGE
#define RHEOSCAPE_EXPONENTIAL_MOVING_AVERAGE

#include <functional>
#include <core_types.hpp>
#include <types/TSValue.hpp>
#include <operators/map.hpp>
#include <operators/reduce.hpp>
#include <operators/zip.hpp>

// Smooth an input reading over a moving average time interval,
// using the exponential moving average or single-pole IIR method.
// If you think of this process as a low-pass filter,
// the time constant is the period of the cutoff frequency.
// Any movements slower than this will get integrated;
// any movements faster than this will get filtered out.
template <typename TCalc, typename TTime>
source_fn<TCalc> exponentialMovingAverage_(source_fn<TCalc> source, source_fn<TTime> clockSource, TTime timeConstant) {
  source_fn<TSValue<TTime, TCalc>> timestamped = zip_<TSValue<TTime, TCalc>, TCalc, TTime>(source, clockSource, [](TCalc value, TTime timestamp) { return TSValue<TTime, TCalc> { timestamp, value }; });

  source_fn<TSValue<TTime, TCalc>> calculated = reduce_<TSValue<TTime, TCalc>>(timestamped, [timeConstant](TSValue<TTime, TCalc> prev, TSValue<TTime, TCalc> next) {
    TTime timeDelta = next.time - prev.time;
    TCalc alpha = 1 - pow(M_E, -timeDelta / timeConstant);
    TCalc integrated = prev.value + alpha * (next.value - prev.value);
    return TSValue<TTime, TCalc> { next.time, integrated };
  });
  
  return map_<TCalc, TSValue<TTime, TCalc>>(calculated, [](TSValue<TTime, TCalc> value) { return value.value; });
}

template <typename TCalc, typename TTime>
pipe_fn<TCalc, TCalc> exponentialMovingAverage(source_fn<TTime> clockSource, TTime timeConstant) {
  return [clockSource, timeConstant](source_fn<TCalc> source) {
    return exponentialMovingAverage_<TCalc, TTime>(source, clockSource, timeConstant);
  };
}

#endif