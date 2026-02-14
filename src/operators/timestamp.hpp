#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>
#include <operators/combine.hpp>
#include <operators/map_tuple.hpp>

namespace rheo::operators {

  template <typename SourceT, typename ClockSourceT>
    requires concepts::Source<SourceT> && concepts::Source<ClockSourceT>
  auto timestamp(SourceT source, ClockSourceT clock_source) {
    using T = source_value_t<SourceT>;
    using TTimePoint = source_value_t<ClockSourceT>;

    struct Tagger {
      RHEO_CALLABLE TaggedValue<T, TTimePoint> operator()(T value, TTimePoint timestamp) const {
        return TaggedValue<T, TTimePoint>{ value, timestamp };
      }
    };

    return combine(std::move(source), std::move(clock_source))
      | map_tuple(Tagger{});
  }

  namespace detail {
    template <typename ClockSourceT>
    struct TimestampPipeFactory {
      ClockSourceT clock_source;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEO_CALLABLE auto operator()(SourceT source) const {
        return timestamp(std::move(source), ClockSourceT(clock_source));
      }
    };
  }

  template <typename ClockSourceT>
    requires concepts::Source<ClockSourceT>
  auto timestamp(ClockSourceT clock_source) {
    return detail::TimestampPipeFactory<ClockSourceT>{std::move(clock_source)};
  }

}
