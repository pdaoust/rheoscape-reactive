#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <operators/timestamp.hpp>
#include <types/Wrapper.hpp>

namespace rheoscape::operators {

  // Don't emit a value until it's settled down.
  // This differs from debounce, in that debounce waits for an upstream change,
  // ignores intermediate values for the duration of the interval,
  // and then emits the new value if the upstream value is the same as it was at the start of the change.
  // settle, on the other hand, discards prior values if a new value comes in,
  // and only emits a changed value if it remains unchanged for the duration of the interval.

  namespace detail {
    template <typename TimestampedSourceT, typename T, typename TTimePoint, typename TInterval>
    struct SettleSourceBinder {
      using value_type = T;

      TimestampedSourceT timestamped;
      TInterval interval;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        // We only want to push a value that's the same as the previously pushed value
        // if the push happens as a consequence of a pull.
        // It doesn't make sense for a push stream to force a settled value to get emitted
        // for every single bouncy new value that's getting discarded.
        auto did_pull = make_wrapper_shared(false);

        struct PushHandler {
          TInterval interval;
          PushFn push;
          std::shared_ptr<Wrapper<bool>> did_pull;
          mutable std::optional<std::tuple<T, TTimePoint>> testing_new_state;
          mutable std::optional<T> current_state;
          mutable bool current_state_has_been_pushed;

          RHEOSCAPE_CALLABLE void operator()(std::tuple<T, TTimePoint> value) const {
            if (
              !testing_new_state.has_value()
              || value.value != testing_new_state.value().value) {
              // New value has come in.
              // Reset the test period.
              testing_new_state = std::optional(value);
            }

            if (value.tag - testing_new_state.value().tag >= interval) {
              // Passed the settling period.
              // This is the new value.
              current_state = std::optional(value.value);
            }

            // Guard against initial non-state.
            if (current_state.has_value() && (!current_state_has_been_pushed || did_pull->value)) {
              // Push the current, not-bouncy state, whatever it is.
              push(current_state.value());
              current_state_has_been_pushed = true;
            }
          }
        };

        struct PullHandler {
          std::shared_ptr<Wrapper<bool>> did_pull;
          pull_fn inner_pull;

          RHEOSCAPE_CALLABLE void operator()() const {
            did_pull->value = true;
            inner_pull();
            did_pull->value = false;
          }
        };

        pull_fn inner_pull = timestamped(PushHandler{
          interval,
          std::move(push),
          did_pull,
          std::nullopt,
          std::nullopt,
          false
        });

        return PullHandler{did_pull, std::move(inner_pull)};
      }
    };
  }

  template <typename SourceT, typename ClockSourceT, typename TInterval>
    requires concepts::Source<SourceT> && concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TInterval>
  auto settle(SourceT source, ClockSourceT clock_source, TInterval interval) {
    using T = source_value_t<SourceT>;
    using TTimePoint = source_value_t<ClockSourceT>;

    auto timestamped = timestamp(std::move(source), std::move(clock_source));
    return detail::SettleSourceBinder<decltype(timestamped), T, TTimePoint, TInterval>{
      std::move(timestamped), interval
    };
  }

  namespace detail {
    template <typename ClockSourceT, typename TInterval>
    struct SettlePipeFactory {
      ClockSourceT clock_source;
      TInterval interval;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return settle(std::move(source), ClockSourceT(clock_source), interval);
      }
    };
  }

  template <typename ClockSourceT, typename TInterval>
    requires concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TInterval>
  auto settle(ClockSourceT clock_source, TInterval interval) {
    return detail::SettlePipeFactory<ClockSourceT, TInterval>{std::move(clock_source), interval};
  }

}
