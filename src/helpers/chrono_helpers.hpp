#pragma once

#include <chrono>
#include <types/core_types.hpp>
#include <operators/lift.hpp>

namespace rheoscape::helpers {

  template <class TClock>
  map_fn<typename TClock::time_point, typename TClock::duration::rep> convert_to_chrono_time_point() {
    struct Converter {
      RHEOSCAPE_CALLABLE typename TClock::time_point operator()(typename TClock::duration::rep value) const {
        return typename TClock::time_point(typename TClock::duration(value));
      }
    };

    return Converter{};
  }

  template <class TDuration>
  map_fn<TDuration, typename TDuration::rep> convert_to_chrono_duration() {
    struct Converter {
      RHEOSCAPE_CALLABLE TDuration operator()(typename TDuration::rep value) const {
        return TDuration(value);
      }
    };

    return Converter{};
  }

  template <class TClock>
  map_fn<typename TClock::duration, typename TClock::time_point> convert_time_point_to_duration() {
    struct Converter {
      RHEOSCAPE_CALLABLE typename TClock::duration operator()(typename TClock::time_point value) const {
        return value.time_since_epoch();
      }
    };

    return Converter{};
  }

  template <typename TTimePoint>
  map_fn<typename TTimePoint::duration::rep, TTimePoint> strip_chrono() {
    struct Converter {
      RHEOSCAPE_CALLABLE typename TTimePoint::duration::rep operator()(TTimePoint v) const {
        return v.time_since_epoch().count();
      }
    };

    return Converter{};
  }

  template <class TDuration>
  map_fn<typename TDuration::rep, TDuration> strip_chrono() {
    struct Converter {
      RHEOSCAPE_CALLABLE typename TDuration::rep operator()(TDuration v) const {
        return v.count();
      }
    };

    return Converter{};
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
