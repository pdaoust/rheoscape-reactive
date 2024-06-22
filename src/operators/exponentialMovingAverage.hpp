#ifndef RHEOSCAPE_EXPONENTIAL_MOVING_AVERAGE
#define RHEOSCAPE_EXPONENTIAL_MOVING_AVERAGE

#include <functional>
#include <core_types.hpp>
#include <operators/map.hpp>
#include <operators/reduce.hpp>
#include <operators/zip.hpp>

// Smooth an input reading over a moving average time interval,
// using the exponential moving average or single-pole IIR method.
// If you think of this process as a low-pass filter,
// the time constant is the period of the cutoff frequency.
// Any movements slower than this will get integrated;
// any movements faster than this will get filtered out.
template <typename TSource, typename TTime, typename TInterval>
source_fn<TSource> exponentialMovingAverage_(source_fn<TSource> source, source_fn<TTime> clockSource, TInterval timeConstant) {
  source_fn<std::tuple<TSource, TTime>> timestamped = zip_<TSource, TTime, std::tuple<TSource, TTime>>(source, clockSource);
  source_fn<std::tuple<TSource, TTime>> calculated = reduce_<std::tuple<TSource, TTime>>(timestamped, [timeConstant](auto prev, auto next) {
    TSource prevValue = std::get<0>(prev);
    TTime prevTS = std::get<1>(prev);
    TSource nextValue = std::get<0>(next);
    TTime nextTS = std::get<1>(next);

    TInterval timeDelta = nextTS - prevTS;
    auto alpha = 1 - pow(M_E, -timeDelta / timeConstant);
    TSource integrated = prevValue + alpha * (nextValue - prevValue);
    return std::tuple<TSource, TTime>(integrated, nextTS);
  });
  return map_<std::tuple<TSource, TTime>, TSource>(calculated, [](auto value) { return std::get<0>(value); });
}

template <typename TSource, typename TTime, typename TInterval>
pipe_fn<TSource, TSource> exponentialMovingAverage(source_fn<TTime> clockSource, TInterval timeConstant) {
  return [clockSource, timeConstant](source_fn<TSource> source) {
    return exponentialMovingAverage_(source, clockSource, timeConstant);
  };
}

#endif