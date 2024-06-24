#ifndef RHEOSCAPE_CHRONO_HELPERS
#define RHEOSCAPE_CHRONO_HELPERS

#include <chrono>
#include <functional>
#include <core_types.hpp>
#include <operators/lift.hpp>

template <class TClock, class Rep, typename Period>
map_fn<std::chrono::time_point<TClock, std::chrono::duration<Rep, Period>>, Rep> convertToChronoTimePoint() {
  return [](Rep value) { return std::chrono::time_point<TClock, std::chrono::duration<Rep, Period>>(value); };
}

template <class Rep, class Period>
map_fn<std::chrono::duration<Rep, Period>, Rep> convertToChronoDuration() {
  return [](Rep value) { return std::chrono::duration<Rep, Period>(value); };
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