#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <operators/timestamp.hpp>

namespace rheoscape::operators {

  // Only allow at most one value per interval,
  // dropping everything in between.
  // Not guaranteed to be precisely spaced by the interval.

  namespace detail {
    template <typename TimestampedSourceT, typename T, typename TTimePoint, typename TInterval>
    struct ThrottleSourceBinder {
      using value_type = T;

      TimestampedSourceT timestamped;
      TInterval interval;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          TInterval interval;
          PushFn push;
          mutable std::optional<TTimePoint> interval_start;

          RHEOSCAPE_CALLABLE void operator()(TaggedValue<T, TTimePoint> value) const {
            if (interval_start.has_value() && value.tag - interval_start.value() > interval) {
              interval_start = std::nullopt;
            }

            if (!interval_start.has_value()) {
              interval_start = value.tag;
              push(value.value);
            }
          }
        };

        return timestamped(PushHandler{interval, std::move(push), std::nullopt});
      }
    };
  }

  template <typename SourceT, typename ClockSourceT, typename TInterval>
    requires concepts::Source<SourceT> && concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TInterval>
  auto throttle(SourceT source, ClockSourceT clock_source, TInterval interval) {
    using T = source_value_t<SourceT>;
    using TTimePoint = source_value_t<ClockSourceT>;

    auto timestamped = timestamp(std::move(source), std::move(clock_source));
    return detail::ThrottleSourceBinder<decltype(timestamped), T, TTimePoint, TInterval>{
      std::move(timestamped), interval
    };
  }

  namespace detail {
    template <typename ClockSourceT, typename TInterval>
    struct ThrottlePipeFactory {
      ClockSourceT clock_source;
      TInterval interval;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return throttle(std::move(source), ClockSourceT(clock_source), interval);
      }
    };
  }

  template <typename ClockSourceT, typename TInterval>
    requires concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TInterval>
  auto throttle(ClockSourceT clock_source, TInterval interval) {
    return detail::ThrottlePipeFactory<ClockSourceT, TInterval>{std::move(clock_source), interval};
  }

}
