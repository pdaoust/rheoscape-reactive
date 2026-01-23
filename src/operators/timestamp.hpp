#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>

namespace rheo::operators {

  template <typename T, typename TTimePoint>
  source_fn<TaggedValue<T, TTimePoint>> timestamp(source_fn<T> source, source_fn<TTimePoint> clock_source) {
    return combine(
      [](T value, TTimePoint timestamp) { return TaggedValue<T, TTimePoint> { value, timestamp }; },
      source,
      clock_source
    );
  }

  template <typename T, typename TTimePoint>
  pipe_fn<TaggedValue<T, TTimePoint>, T> timestamp(source_fn<TTimePoint> clock_source) {
    return [clock_source](source_fn<T> source) {
      return timestamp(source, clock_source);
    };
  }

}