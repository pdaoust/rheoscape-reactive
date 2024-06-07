#ifndef TIMESTAMPED_VALUE_HPP
#define TIMESTAMPED_VALUE_HPP

#include <chrono>

typedef std::chrono::steady_clock::duration mono_duration;
typedef std::chrono::steady_clock::time_point mono_time_point;

typedef std::chrono::system_clock::duration wall_duration;
typedef std::chrono::system_clock::time_point wall_time_point;

template <typename TTime, typename TVal>
struct TSValue {
  TTime time;
  TVal value;

  TSValue(TTime time, TVal value)
  :
    time(time),
    value(value)
  { }
};

template <typename T>
struct MTSValue : TSValue<mono_time_point, T> {};

template <typename T>
struct WTSValue : TSValue<wall_time_point, T> {};

#endif