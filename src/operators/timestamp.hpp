#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>
#include <operators/map_tuple.hpp>

namespace rheo::operators {

  template <typename T, typename TTimePoint>
  source_fn<TaggedValue<T, TTimePoint>> timestamp(source_fn<T> source, source_fn<TTimePoint> clock_source) {
    return combine(source, clock_source)
      | map_tuple([](T value, TTimePoint timestamp) {
          return TaggedValue<T, TTimePoint> { value, timestamp };
        });
  }

  template <typename T, typename TTimePoint>
  pipe_fn<TaggedValue<T, TTimePoint>, T> timestamp(source_fn<TTimePoint> clock_source) {
    return [clock_source](source_fn<T> source) {
      return timestamp(source, clock_source);
    };
  }

}