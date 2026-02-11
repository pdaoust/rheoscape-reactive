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

  template <typename T, typename TTimePoint, typename TInterval>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TInterval>
  source_fn<T> timed_latch(source_fn<T> source, source_fn<TTimePoint> clock_source, TInterval duration, T default_value) {

    struct SourceBinder {
      source_fn<T> source;
      source_fn<TTimePoint> clock_source;
      TInterval duration;
      T default_value;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
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
          push_fn<T> push;
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

    return SourceBinder{
      std::move(source),
      std::move(clock_source),
      duration,
      std::move(default_value)
    };
  }

  template <typename T, typename TTimePoint, typename TInterval>
    requires concepts::TimePointAndDurationCompatible<TTimePoint, TInterval>
  pipe_fn<T, T> timed_latch(source_fn<TTimePoint> clock_source, TInterval duration, T default_value) {

    struct PipeFactory {
      source_fn<TTimePoint> clock_source;
      TInterval duration;
      T default_value;

      RHEO_CALLABLE source_fn<T> operator()(source_fn<T> source) const {
        return timed_latch(source, clock_source, duration, default_value);
      }
    };

    return PipeFactory{clock_source, duration, default_value};
  }

}
