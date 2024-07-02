#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/zip.hpp>

namespace rheo {

  template <typename T, typename TTime>
  source_fn<T> timestamp(source_fn<T> source, source_fn<TTime> clockSource) {
    return zip(source, clockSource, [](T value, TTime timestamp) { return TSValue<TTime, T> { timestamp, value }; });
  }

}