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
      (combine2_fn<TaggedValue<T, TTime>, T, TTime>)[](T value, TTime timestamp) { return TaggedValue<T, TTime> { value, timestamp }; }
    );
  }

}