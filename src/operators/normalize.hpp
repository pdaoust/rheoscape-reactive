#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Range.hpp>
#include <operators/zip.hpp>

namespace rheo {

  template <typename TIn, typename TOut>
  source_fn<TOut> normalize(source_fn<TIn> source, source_fn<Range<TIn>> fromSource, source_fn<Range<TOut>> toSource) {
    return zip3(
      source,
      fromSource,
      toSource,
      [](TIn value, Range<TIn> from, Range<TOut> to) {
        if (value <= from.min) {
          return to.min;
        } else if (value >= from.max) {
          return to.max;
        } else {
          return to.min + (to.max - to.min) * (value - from.min) / (from.max - from.min);
        }
      }
    );
  }

}