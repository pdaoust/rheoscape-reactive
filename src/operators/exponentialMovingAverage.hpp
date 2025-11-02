#pragma once

#include <functional>
#include <core_types.hpp>
#include <util.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>
#include <operators/scan.hpp>
#include <operators/timestamp.hpp>

namespace rheo::operators {

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
    source_fn<TInterval> timeConstantSource,
    TMapIntervalToRep&& mapIntervalToRep
  ) {
    using TRep = transformer_1_in_out_type_t<std::decay_t<TMapIntervalToRep>>;

    return combine(source, timeConstantSource, std::make_tuple<TVal, TInterval>)
      | timestamp<std::tuple<TVal, TInterval>>(clockSource)
      | scan(
        std::optional<TaggedValue<TVal, TTime>>{},
        [mapIntervalToRep = std::forward<TMapIntervalToRep>(mapIntervalToRep)]
        (std::optional<TaggedValue<TVal, TTime>> prev, TaggedValue<std::tuple<TVal, TInterval>, TTime> next) {
          TVal nextValue = std::get<0>(next.value);

          if (!prev.has_value()) {
            // First run, no average to be taken.
            return std::optional<TaggedValue<TVal, TTime>> { { nextValue, next.tag } };
          }

          TVal prevValue = prev.value().value;
          TInterval timeConstant = std::get<1>(next.value);
          TInterval timeDelta = next.tag - prev.value().tag;
          TRep alpha = (TRep)1.0 - pow((TRep)M_E, -mapIntervalToRep(timeDelta) / mapIntervalToRep(timeConstant));
          TVal integrated = prevValue + (nextValue - prevValue) * alpha;
          return std::optional<TaggedValue<TVal, TTime>> { { integrated, next.tag } };
        }
      )
      | map([](std::optional<TaggedValue<TVal, TTime>> value) { return value.value().value; });
  }

  // A simpler version for use with scalar time rather than std::chrono time.
  template <typename TVal, typename TTime>
  source_fn<TVal> exponentialMovingAverage(source_fn<TVal> source, source_fn<TTime> clockSource, source_fn<TTime> timeConstantSource) {
    return exponentialMovingAverage(
      source,
      clockSource,
      timeConstantSource,
      [](TTime t) { return (TVal)t; }
    );
  }

  template <typename TVal, typename TTime, typename TInterval, typename TMapIntervalToRep>
  pipe_fn<TVal, TVal> exponentialMovingAverage(source_fn<TTime> clockSource, source_fn<TTime> timeConstantSource, TMapIntervalToRep&& mapIntervalToRep) {
    return [clockSource, timeConstantSource, mapIntervalToRep = std::forward<TMapIntervalToRep>(mapIntervalToRep)](source_fn<TVal> source) {
      return exponentialMovingAverage(source, clockSource, timeConstantSource, mapIntervalToRep);
    };
  }

  template <typename TVal, typename TTime>
  pipe_fn<TVal, TVal> exponentialMovingAverage(source_fn<TTime> clockSource, source_fn<TTime> timeConstantSource) {
    return exponentialMovingAverage(clockSource, timeConstantSource, [](TTime t) { return (TVal)t; });
  }

  template <typename TRep, typename TInterval>
  TRep mapChronoToScalar(TInterval interval) {
    return (TRep)interval.count();
  }

}