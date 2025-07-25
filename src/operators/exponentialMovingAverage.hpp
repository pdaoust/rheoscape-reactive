#pragma once

#include <functional>
#include <core_types.hpp>
#include <util.hpp>
#include <types/TaggedValue.hpp>
#include <operators/map.hpp>
#include <operators/scan.hpp>
#include <operators/timestamp.hpp>

namespace rheo::operators {

  template <typename TRep>
  TRep mapIntervalIsRep(TRep interval) {
    return interval;
  }

  // Smooth an input reading over a moving average time interval,
  // using the exponential moving average or single-pole IIR method.
  // If you think of this process as a low-pass filter,
  // the time constant is the period of the cutoff frequency.
  // Any movements slower than this will get integrated;
  // any movements faster than this will get filtered out.
  template <typename TVal, typename TTime, typename TInterval, typename TMapIntervalToRep>
  source_fn<TVal> exponentialMovingAverage(
    source_fn<TVal> source,
    source_fn<TTime> clockSource,
    TInterval timeConstant,
    TMapIntervalToRep&& mapIntervalToRep
  ) {
    using TRep = transformer_1_in_out_type_t<std::decay_t<TMapIntervalToRep>>;

    return timestamp(source, clockSource)
      | scan([timeConstant, mapIntervalToRep = std::forward<TMapIntervalToRep>(mapIntervalToRep)](TaggedValue<TVal, TTime> prev, TaggedValue<TVal, TTime> next) {
        TInterval timeDelta = next.tag - prev.tag;
        TRep alpha = (TRep)1.0 - pow((TRep)M_E, -mapIntervalToRep(timeDelta) / mapIntervalToRep(timeConstant));
        TVal integrated = prev.value + (next.value - prev.value) * alpha;
        return TaggedValue<TVal, TTime> { integrated, next.tag };
      })
      | map([](TaggedValue<TVal, TTime> value) { return value.value; });
  }

  // A simpler version for use with scalar time rather than std::chrono time.
  template <typename TVal, typename TTime>
  source_fn<TVal> exponentialMovingAverage(source_fn<TVal> source, source_fn<TTime> clockSource, TVal timeConstant) {
    return exponentialMovingAverage(
      source,
      clockSource,
      timeConstant,
      mapIntervalIsRep<TVal>
    );
  }

  template <typename TVal, typename TTime, typename TInterval, typename TMapIntervalToRep>
  pipe_fn<TVal, TVal> exponentialMovingAverage(source_fn<TTime> clockSource, TInterval timeConstant, TMapIntervalToRep&& mapIntervalToRep) {
    return [clockSource, timeConstant, mapIntervalToRep = std::forward<TMapIntervalToRep>(mapIntervalToRep)](source_fn<TVal> source) {
      return exponentialMovingAverage(source, clockSource, timeConstant, mapIntervalToRep);
    };
  }

  template <typename TVal, typename TTime>
  pipe_fn<TVal, TVal> exponentialMovingAverage(source_fn<TTime> clockSource, TVal timeConstant) {
    return exponentialMovingAverage(clockSource, timeConstant, mapIntervalIsRep<TVal>);
  }

  template <typename TRep, typename TInterval>
  TRep mapChronoToScalar(TInterval interval) {
    return (TRep)interval.count();
  }

}