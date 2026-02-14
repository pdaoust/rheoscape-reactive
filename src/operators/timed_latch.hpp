#pragma once

#include <functional>
#include <memory>
#include <core_types.hpp>

namespace rheo::operators {

  // When the passed source changes,
  // hold a value for a while before reverting to a default value.
  // The initial non-default value will be pushed,
  // then the default value will be pushed at the end of the latch duration
  // (unless upstream has some new value, in which case it'll latch to that instead --
  // and if the new value is the same as the value that was just latched,
  // then it'll stay latched to that for another latch duration).
  // Of course you can pull it as many times as you like inside or out of a latch
  // to get the latched or default value too.
  // If the value changes to the default or another value
  // before the latch period is up,
  // you'll still get the value that triggered the latch period
  // rather than whatever it is now.

  namespace detail {
    template <typename SourceT, typename ClockSourceT, typename TInterval>
    struct TimedLatchSourceBinder {
      using T = source_value_t<SourceT>;
      using TTimePoint = source_value_t<ClockSourceT>;
      using value_type = T;

      SourceT source;
      ClockSourceT clock_source;
      TInterval duration;
      T default_value;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        auto last_timestamp = std::make_shared<std::optional<TTimePoint>>();

        struct ClockPushHandler {
          std::shared_ptr<std::optional<TTimePoint>> last_timestamp;

          RHEO_CALLABLE void operator()(TTimePoint ts) const {
            last_timestamp->emplace(ts);
          }
        };

        // Rather than use the combine() operator,
        // which would pull a T value every time the clock pushes,
        // we only pull the clock when a new T is pushed.
        // So first we bind to the clock source here,
        // then we pull from the clock source in the push function.
        pull_fn pull_clock = clock_source(ClockPushHandler{last_timestamp});

        // Shared pointers needed to share state between bindings.
        auto latch_start_timestamp = std::make_shared<std::optional<TTimePoint>>();
        auto last_value = std::make_shared<std::optional<T>>();

        struct PushHandler {
          TInterval duration;
          T default_value;
          PushFn push;
          std::shared_ptr<std::optional<TTimePoint>> last_timestamp;
          pull_fn pull_clock;
          std::shared_ptr<std::optional<TTimePoint>> latch_start_timestamp;
          std::shared_ptr<std::optional<T>> last_value;

          RHEO_CALLABLE void operator()(T value) const {
            pull_clock();

            if (latch_start_timestamp->has_value()) {
              if (last_timestamp->value() - latch_start_timestamp->value() < duration) {
                // Within the latch period; no changes.
                push(last_value->value());
                return;
              }

              // Expired; reset latch counter.
              latch_start_timestamp->reset();
            }

            // Either the latch duration has expired or the latch has never been triggered.
            // If it's a non-default value, start a new latch interval.
            if (value != default_value) {
              latch_start_timestamp->emplace(last_timestamp->value());
              last_value->emplace(value);
            }

            push(value);
          }
        };

        return source(PushHandler{
          duration,
          default_value,
          std::move(push),
          last_timestamp,
          std::move(pull_clock),
          latch_start_timestamp,
          last_value
        });
      }
    };
  }

  template <typename SourceT, typename ClockSourceT, typename TInterval>
    requires concepts::Source<SourceT> && concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TInterval>
  auto timed_latch(SourceT source, ClockSourceT clock_source, TInterval duration, source_value_t<SourceT> default_value) {
    return detail::TimedLatchSourceBinder<SourceT, ClockSourceT, TInterval>{
      std::move(source),
      std::move(clock_source),
      duration,
      std::move(default_value)
    };
  }

  namespace detail {
    template <typename T, typename ClockSourceT, typename TInterval>
    struct TimedLatchPipeFactory {
      ClockSourceT clock_source;
      TInterval duration;
      T default_value;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEO_CALLABLE auto operator()(SourceT source) const {
        return timed_latch(std::move(source), ClockSourceT(clock_source), duration, T(default_value));
      }
    };
  }

  template <typename ClockSourceT, typename TInterval, typename T>
    requires concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TInterval>
  auto timed_latch(ClockSourceT clock_source, TInterval duration, T default_value) {
    return detail::TimedLatchPipeFactory<T, ClockSourceT, TInterval>{
      std::move(clock_source), duration, std::move(default_value)
    };
  }

}
