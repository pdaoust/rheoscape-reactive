#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>

namespace rheo::operators {

  template <typename T, typename TTime>
  source_fn<TaggedValue<T, TTime>> timestamp(source_fn<T> source, source_fn<TTime> clockSource) {
    return combine(
      source,
      clockSource,
      [](T value, TTime timestamp) { return TaggedValue<T, TTime> { value, timestamp }; }
    );
  }

  template <typename T, typename TTime>
  pipe_fn<TaggedValue<T, TTime>, T> timestamp(source_fn<TTime> clockSource) {
    return [clockSource](source_fn<T> source) {
      return timestamp(source, clockSource);
    };
  }

}