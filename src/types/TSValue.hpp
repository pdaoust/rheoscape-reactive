#pragma once

#include <core_types.hpp>

namespace rheo {

  template <typename TTime, typename TVal>
  struct TSValue {
    const TTime time;
    const TVal value;

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

}