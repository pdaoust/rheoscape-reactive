#pragma once

#include <chrono>
#include <functional>
#include <core_types.hpp>
#include <operators/lift.hpp>

namespace rheo::operators {

  template <class TClock>
  map_fn<typename TClock::time_point, typename TClock::duration::rep> convert_to_chrono_time_point() {
    return [](typename TClock::duration::rep value) { return TClock::time_point(value); };
  }

  template <class TDuration>
  map_fn<TDuration, typename TDuration::rep> convert_to_chrono_duration() {
    return [](typename TDuration::rep value) { return TDuration(value); };
  }

  template <class TClock>
  map_fn<typename TClock::duration, typename TClock::time_point> convert_time_point_to_duration() {
    return [](typename TClock::time_point value) { return value.time_since_epoch(); };
  }

  template <typename TTimePoint>
  map_fn<typename TTimePoint::duration::rep, TTimePoint> strip_chrono() {
    return [](TTimePoint v) { return v.time_since_epoch().count(); };
  }

  template <class TDuration>
  map_fn<typename TDuration::rep, TDuration> strip_chrono() {
    return [](TDuration v) { return v.count(); };
  }

  template <typename TClock>
  pipe_fn<typename TClock::time_point, typename TClock::time_point> lift_to_chrono_time_point(pipe_fn<typename TClock::duration::rep, typename TClock::duration::rep> scalar_pipe) {
    return lift(
      scalar_pipe,
      convert_to_chrono_time_point<TClock>(),
      strip_chrono<typename TClock::time_point>()
    );
  }

  template <typename TDuration>
  pipe_fn<TDuration, TDuration> lift_to_chrono_duration(pipe_fn<typename TDuration::rep, typename TDuration::rep> scalar_pipe) {
    return lift(
      scalar_pipe,
      convert_to_chrono_duration<TDuration>(),
      strip_chrono<TDuration>()
    );
  }

}