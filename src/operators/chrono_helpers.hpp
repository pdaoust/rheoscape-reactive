#pragma once

#include <chrono>
#include <functional>
#include <core_types.hpp>
#include <operators/lift.hpp>

namespace rheo::operators {

  template <class TClock>
  map_fn<typename TClock::time_point, typename TClock::duration::rep> convertToChronoTimePoint() {
    return [](typename TClock::duration::rep value) { return TClock::time_point(value); };
  }

  template <class TDuration>
  map_fn<TDuration, typename TDuration::rep> convertToChronoDuration() {
    return [](typename TDuration::rep value) { return TDuration(value); };
  }

  template <class TClock>
  map_fn<typename TClock::duration, typename TClock::time_point> convertTimePointToDuration() {
    return [](typename TClock::time_point value) { return value.time_since_epoch(); };
  }

  template <typename TTimePoint>
  map_fn<typename TTimePoint::duration::rep, TTimePoint> stripChrono() {
    return [](TTimePoint v) { return v.time_since_epoch().count(); };
  }

  template <class TDuration>
  map_fn<typename TDuration::rep, TDuration> stripChrono() {
    return [](TDuration v) { return v.count(); };
  }

  template <typename TClock>
  pipe_fn<typename TClock::time_point, typename TClock::time_point> liftToChronoTimePoint(pipe_fn<typename TClock::duration::rep, typename TClock::duration::rep> scalarPipe) {
    return lift(
      scalarPipe,
      convertToChronoTimePoint<TClock>(),
      stripChrono<typename TClock::time_point>()
    );
  }

  template <typename TDuration>
  pipe_fn<TDuration, TDuration> liftToChronoDuration(pipe_fn<typename TDuration::rep, typename TDuration::rep> scalarPipe) {
    return lift(
      scalarPipe,
      convertToChronoDuration<TDuration>(),
      stripChrono<TDuration>()
    );
  }

}