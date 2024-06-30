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
template <typename TVal, typename TTime, typename TInterval, typename TRep>
source_fn<TVal> exponentialMovingAverage_(source_fn<TVal> source, source_fn<TTime> clockSource, TInterval timeConstant) {
  source_fn<TSValue<TTime, TVal>> timestamped = zip_<TSValue<TTime, TVal>, TVal, TTime>(
    source,
    clockSource,
    [](TVal value, TTime timestamp) {
      return TSValue<TTime, TVal>{ timestamp, value };
    }
  );

  source_fn<TSValue<TTime, TVal>> calculated = reduce_<TSValue<TTime, TVal>>(
    timestamped,
    [timeConstant](TSValue<TTime, TVal> prev, TSValue<TTime, TVal> next) {
      TInterval timeDelta = next.time - prev.time;
      TRep alpha = 1 - pow(M_E, -timeDelta / timeConstant);
      TVal integrated = prev.value + alpha * (next.value - prev.value);
      return TSValue<TTime, TVal>{ std::move(next.time), std::move(integrated) };
    }
  );
  
  return map_<TVal, TSValue<TTime, TVal>>(calculated, [](TSValue<TTime, TVal> value) { return value.value; });
}

template <typename TVal, typename TTime, typename TInterval, typename TRep>
pipe_fn<TVal, TVal> exponentialMovingAverage(source_fn<TTime> clockSource, TInterval timeConstant) {
  return [clockSource, timeConstant](source_fn<TVal> source) {
    return exponentialMovingAverage_<TVal, TTime, TInterval, TRep>(source, clockSource, timeConstant);
  };
}

#endif