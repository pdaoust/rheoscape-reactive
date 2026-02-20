#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>

namespace rheoscape::operators {

  template <typename SourceT, typename ClockSourceT>
    requires concepts::Source<SourceT> && concepts::Source<ClockSourceT>
  auto timestamp(SourceT source, ClockSourceT clock_source) {
    using T = source_value_t<SourceT>;
    using TTimePoint = source_value_t<ClockSourceT>;

    struct Tagger {
      RHEOSCAPE_CALLABLE std::tuple<T, TTimePoint> operator()(T value, TTimePoint timestamp) const {
        return std::tuple<T, TTimePoint>{ value, timestamp };
      }
    };

    return combine(std::move(source), std::move(clock_source))
      | map(Tagger{});
  }

  namespace detail {
    template <typename ClockSourceT>
    struct TimestampPipeFactory {
      ClockSourceT clock_source;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
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
