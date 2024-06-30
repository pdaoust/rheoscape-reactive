#ifndef RHEOSCAPE_CHRONO_HELPERS
#define RHEOSCAPE_CHRONO_HELPERS

#include <chrono>
#include <functional>
#include <core_types.hpp>
#include <operators/lift.hpp>

template <class TClock>
map_fn<typename TClock::time_point, typename TClock::duration::rep> convertToChronoTimePoint() {
  return [](typename TClock::duration::rep value) { return TClock::time_point(value); };
}

template <class TClock>
map_fn<typename TClock::duration, typename TClock::duration::rep> convertToChronoDuration() {
  return [](typename TClock::duration::rep value) { return TClock::duration(value); };
}

template <class TClock>
map_fn<typename TClock::duration, typename TClock::time_point> convertTimePointToDuration() {
  return [](typename TClock::time_point value) { return value.time_since_epoch(); };
}

template <typename TClock, class Rep, class Period>
map_fn<Rep, std::chrono::time_point<TClock, std::chrono::duration<Rep, Period>>> stripChrono() {
  return [](std::chrono::time_point<TClock, std::chrono::duration<Rep, Period>> v) { return v.time_since_epoch().count(); };
}

template <class Rep, class Period>
map_fn<Rep, std::chrono::duration<Rep, Period>> stripChrono() {
  return [](std::chrono::duration<Rep, Period> v) { return v.count(); };
}

template <typename TClock, typename Rep, typename Period>
pipe_fn<std::chrono::time_point<TClock, std::chrono::duration<Rep, Period>>, std::chrono::time_point<TClock, std::chrono::duration<Rep, Period>>> liftToChronoTimePoint(pipe_fn<Rep, Rep> scalarPipe) {
  return lift(
    scalarPipe,
    convertToChronoTimePoint<TClock, Rep, Period>(),
    stripChrono<TClock, Rep, Period>()
  );
}

template <typename Rep, typename Period>
pipe_fn<std::chrono::duration<Rep, Period>, std::chrono::duration<Rep, Period>> liftToChronoDuration(pipe_fn<Rep, Rep> scalarPipe) {
  return lift(
    scalarPipe,
    convertToChronoDuration<Rep, Period>(),
    stripChrono<Rep, Period>()
  );
}

#endif