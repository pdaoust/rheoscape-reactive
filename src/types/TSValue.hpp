#ifndef RHEOSCAPE_TSVALUE
#define RHEOSCAPE_TSVALUE

#include <core_types.hpp>

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