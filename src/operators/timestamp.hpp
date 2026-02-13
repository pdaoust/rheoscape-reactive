#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>
#include <operators/map_tuple.hpp>

namespace rheo::operators {

  template <typename T, typename TTimePoint>
  source_fn<TaggedValue<T, TTimePoint>> timestamp(source_fn<T> source, source_fn<TTimePoint> clock_source) {
    struct Tagger {
      RHEO_CALLABLE TaggedValue<T, TTimePoint> operator()(T value, TTimePoint timestamp) const {
        return TaggedValue<T, TTimePoint>{ value, timestamp };
      }
    };

    return combine(source, clock_source)
      | map_tuple(Tagger{});
  }

  namespace detail {
    template <typename TTimePoint>
    struct TimestampPipeFactory {
      source_fn<TTimePoint> clock_source;

      template <typename T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return timestamp(source, clock_source);
      }
    };
  }

  template <typename TTimePoint>
  auto timestamp(source_fn<TTimePoint> clock_source) {
    return detail::TimestampPipeFactory<TTimePoint>{clock_source};
  }

}
