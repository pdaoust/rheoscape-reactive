#pragma once

#include <functional>
#include <core_types.hpp>
#include <util.hpp>
#include <types/TaggedValue.hpp>
#include <operators/map.hpp>
#include <operators/reduce.hpp>
#include <operators/timestamp.hpp>

namespace rheo::operators {

  // Smooth an input reading over a moving average time interval,
  // using the exponential moving average or single-pole IIR method.
  // If you think of this process as a low-pass filter,
  // the time constant is the period of the cutoff frequency.
  // Any movements slower than this will get integrated;
  // any movements faster than this will get filtered out.
  template <typename TVal, typename TTime, typename TInterval, typename TRep>
  source_fn<TVal> exponentialMovingAverage(
    source_fn<TVal> source,
    source_fn<TTime> clockSource,
    TInterval timeConstant,
    map_fn<TRep, TInterval> mapIntervalToRep
  ) {
    auto timestamped = timestamp(source, clockSource);

    source_fn<TaggedValue<TVal, TTime>> calculated = reduce<TaggedValue<TVal, TTime>>(
      timestamped,
      (reduce_fn<TaggedValue<TVal, TTime>>)[timeConstant, mapIntervalToRep](TaggedValue<TVal, TTime> prev, TaggedValue<TVal, TTime> next) {
        TInterval timeDelta = next.tag - prev.tag;
        TRep alpha = (TRep)1.0 - pow((TRep)M_E, -mapIntervalToRep(timeDelta) / mapIntervalToRep(timeConstant));
        TVal integrated = prev.value + (next.value - prev.value) * alpha;
        return TaggedValue<TVal, TTime> { integrated, next.tag };
      }
    );
    
    return map<TVal, TaggedValue<TVal, TTime>>(calculated, [](TaggedValue<TVal, TTime> value) { return value.value; });
  }

  template <typename TVal, typename TTime, typename TInterval, typename TRep>
  pipe_fn<TVal, TVal> exponentialMovingAverage(source_fn<TTime> clockSource, TInterval timeConstant, map_fn<TRep, TInterval> mapIntervalToRep) {
    return [clockSource, timeConstant, mapIntervalToRep](source_fn<TVal> source) {
      return exponentialMovingAverage<TVal, TTime, TInterval, TRep>(source, clockSource, timeConstant, mapIntervalToRep);
    };
  }

  template <typename TRep, typename TInterval>
  TRep mapChronoToScalar(TInterval interval) {
    return (TRep)interval.count();
  }

  template <typename TRep>
  TRep mapIntervalIsRep(TRep interval) {
    return interval;
  }

}