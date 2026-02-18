#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <operators/timestamp.hpp>
#include <types/Wrapper.hpp>

namespace rheoscape::operators {

  // Don't emit a value until it's settled down
  // and matches the value recorded at the beginning of the interval.
  // Note: There is a period at the beginning of the stream
  // in which there will be no values pushed at all,
  // because it's checking whether the upstream source's initial value is stable.

  namespace detail {
    template <typename TimestampedSourceT, typename T, typename TTimePoint, typename TInterval>
    struct DebounceSourceBinder {
      using value_type = T;

      TimestampedSourceT timestamped;
      TInterval interval;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        // We only want to push a value that's the same as the previously pushed value
        // if the push happens as a consequence of a pull.
        // It doesn't make sense for a push stream to force a debounced value to get emitted
        // for every single bouncy new value that's getting discarded.
        auto did_pull = make_wrapper_shared(false);

        struct PushHandler {
          TInterval interval;
          PushFn push;
          std::shared_ptr<Wrapper<bool>> did_pull;
          mutable std::optional<TaggedValue<T, TTimePoint>> testing_new_state;
          mutable std::optional<T> current_state;
          mutable bool current_state_has_been_pushed;

          RHEOSCAPE_CALLABLE void operator()(TaggedValue<T, TTimePoint> value) const {
            if (testing_new_state.has_value()
                // NOTE: This equation has to be this way
                // to guard against wraparound for unsigned time representations.
                // https://arduino.stackexchange.com/a/12588
                && value.tag - testing_new_state.value().tag >= interval) {
              // The debounce period has settled down.
              // Has it held the new state value to the end of the settling period?
              if (value.value == testing_new_state.value().value) {
                // Yup, this is now the new state.
                current_state = std::optional(value.value);
              }
              // Reset the testing value in preparation for the next state change.
              testing_new_state = std::nullopt;
            }

            // If the new value isn't the same as the current state,
            // or if there is no current or testing states yet,
            // start a new debounce test period.
            if (
              (!current_state.has_value() || current_state.value() != value.value)
              && !testing_new_state.has_value()
            ) {
              testing_new_state = std::optional(value);
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
  auto debounce(SourceT source, ClockSourceT clock_source, TInterval interval) {
    using T = source_value_t<SourceT>;
    using TTimePoint = source_value_t<ClockSourceT>;

    auto timestamped = timestamp(std::move(source), std::move(clock_source));
    return detail::DebounceSourceBinder<decltype(timestamped), T, TTimePoint, TInterval>{
      std::move(timestamped), interval
    };
  }

  namespace detail {
    template <typename ClockSourceT, typename TInterval>
    struct DebouncePipeFactory {
      ClockSourceT clock_source;
      TInterval interval;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return debounce(std::move(source), ClockSourceT(clock_source), interval);
      }
    };
  }

  template <typename ClockSourceT, typename TInterval>
    requires concepts::Source<ClockSourceT> &&
             concepts::TimePointAndDurationCompatible<source_value_t<ClockSourceT>, TInterval>
  auto debounce(ClockSourceT clock_source, TInterval interval) {
    return detail::DebouncePipeFactory<ClockSourceT, TInterval>{std::move(clock_source), interval};
  }

}
