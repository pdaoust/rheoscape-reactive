#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/timestamp.hpp>

namespace rheo::operators {

  // Only allow at most one value per interval,
  // dropping everything in between.
  // Not guaranteed to be precisely spaced by the interval.

  template <typename T, typename TTimePoint, typename TInterval>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TInterval>
  source_fn<T> throttle(source_fn<T> source, source_fn<TTimePoint> clock_source, TInterval interval) {

    struct SourceBinder {
      source_fn<TaggedValue<T, TTimePoint>> timestamped;
      TInterval interval;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PushHandler {
          TInterval interval;
          push_fn<T> push;
          mutable std::optional<TTimePoint> interval_start;

          RHEO_CALLABLE void operator()(TaggedValue<T, TTimePoint> value) const {
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

    return SourceBinder{
      timestamp(source, clock_source),
      interval
    };
  }

  namespace detail {
    template <typename TTimePoint, typename TInterval>
    struct ThrottlePipeFactory {
      source_fn<TTimePoint> clock_source;
      TInterval interval;

      template <typename T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return throttle(source, clock_source, interval);
      }
    };
  }

  template <typename TTimePoint, typename TInterval>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TInterval>
  auto throttle(source_fn<TTimePoint> clock_source, TInterval interval) {
    return detail::ThrottlePipeFactory<TTimePoint, TInterval>{clock_source, interval};
  }

}
