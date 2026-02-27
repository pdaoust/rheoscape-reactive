#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <util/misc.hpp>
#include <sources/constant.hpp>

namespace rheoscape::operators {

  // Send a timestamp at the interval specified by the interval source.
  // The time source should supply its numbers regularly,
  // as time points that can be subtracted from each other
  // to yield values of the interval source's type.
  // (For Arduino millis, that's unsigned long for both.)
  // The interval source must be pullable,
  // must yield another number to specify the next interval
  // after the previous interval has passed,
  // and should probably not do its own pushing.
  // The simplest interval source is `constant(interval)`.
  // (There's an alias for it called `every`, in this file.)
  // Or, for exponential backoff, you could use the `curve` function.
  //
  // This function ends when either of its sources ends.

  // TODO: could this entire thing be replaced with a filter or a reduce?
  // Something like
  //    filter(time_source, [](ts) { return !(ts % interval); }
  // But with something that would allow it to snap to time intervals if it missed one

  namespace detail {
    template <typename TimeSourceT, typename IntervalSourceT>
    struct IntervalSourceBinder {
      using TTimePoint = source_value_t<TimeSourceT>;
      using TInterval = source_value_t<IntervalSourceT>;
      using value_type = TTimePoint;

      TimeSourceT time_source;
      IntervalSourceT interval_source;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        auto last_interval = std::make_shared<std::optional<TInterval>>();
        auto last_interval_timestamp = std::make_shared<std::optional<TTimePoint>>();

        struct IntervalPushHandler {
          std::shared_ptr<std::optional<TInterval>> last_interval;

          RHEOSCAPE_CALLABLE void operator()(TInterval interval) const {
            last_interval->emplace(interval);
          }
        };

        struct TimePushHandler {
          std::shared_ptr<std::optional<TInterval>> last_interval;
          std::shared_ptr<std::optional<TTimePoint>> last_interval_timestamp;
          pull_fn pull_next_interval;
          PushFn push;

          RHEOSCAPE_CALLABLE void operator()(TTimePoint timestamp) const {
            if (!last_interval->has_value()) {
              pull_next_interval();
              if (!last_interval->has_value()) {
                // Can't start yet; we have no interval.
                return;
              }
            }

            // This comes after we pull the first interval.
            // That's because we don't want to start counting
            // until we know what we're counting from/to.
            if (!last_interval_timestamp->has_value()) {
              // First pull or push of a timestamp; start the thing!
              last_interval_timestamp->emplace(timestamp);
            }

            if (timestamp - last_interval_timestamp->value() >= last_interval->value()) {
              // An interval has passed. Push the timestamp and get the next interval.
              // Don't use the current timestamp; that'll result in uneven interval spacing!
              // Better to have unevenly emitted timestamps than unevenly calculated intervals.
              last_interval_timestamp->emplace(last_interval_timestamp->value() + last_interval->value());
              push(last_interval_timestamp->value());
              pull_next_interval();
            }
          }
        };

        pull_fn pull_next_interval = interval_source(IntervalPushHandler{last_interval});

        return time_source(TimePushHandler{
          last_interval,
          last_interval_timestamp,
          std::move(pull_next_interval),
          std::move(push)
        });
      }
    };
  }

  template <typename TimeSourceT, typename IntervalSourceT>
    requires concepts::Source<TimeSourceT> && concepts::Source<IntervalSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<TimeSourceT>, source_value_t<IntervalSourceT>>
  auto interval(TimeSourceT time_source, IntervalSourceT interval_source) {
    return detail::IntervalSourceBinder<TimeSourceT, IntervalSourceT>{
      std::move(time_source),
      std::move(interval_source)
    };
  }

  namespace detail {
    template <typename IntervalSourceT>
    struct IntervalPipeFactory {
      IntervalSourceT interval_source;

      template <typename TimeSourceT>
        requires concepts::Source<TimeSourceT> &&
                 concepts::TimePointAndDurationCompatible<source_value_t<TimeSourceT>, source_value_t<IntervalSourceT>>
      RHEOSCAPE_CALLABLE auto operator()(TimeSourceT time_source) const {
        return interval(std::move(time_source), IntervalSourceT(interval_source));
      }
    };
  }

  template <typename IntervalSourceT>
    requires concepts::Source<IntervalSourceT>
  auto interval(IntervalSourceT interval_source) {
    return detail::IntervalPipeFactory<IntervalSourceT>{std::move(interval_source)};
  }

  // An alias for `constant`, to make it read better!
  template <typename TInterval>
  source_fn<TInterval> every(TInterval interval) {
    return sources::constant(interval);
  }

  // Useful for exponential backoff.
  // I wouldn't recommend using a factor <1,
  // cuz then it'd just start getting faster and faster!
  template <typename TInterval>
  source_fn<TInterval> curve(TInterval start_interval, float factor) {

    struct SourceBinder {
      TInterval start_interval;
      float factor;

      RHEOSCAPE_CALLABLE pull_fn operator()(push_fn<TInterval> push) const {

        struct PullHandler {
          mutable TInterval acc;
          float factor;
          push_fn<TInterval> push;

          RHEOSCAPE_CALLABLE void operator()() const {
            push(acc);
            acc *= factor;
          }
        };

        return PullHandler{start_interval, factor, std::move(push)};
      }
    };

    return SourceBinder{std::move(start_interval), factor};
  }

}
