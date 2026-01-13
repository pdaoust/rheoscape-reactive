#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/timestamp.hpp>
#include <types/Wrapper.hpp>

namespace rheo::operators {

  // Don't emit a value until it's settled down
  // and matches the value recorded at the beginning of the interval.
  // Note: There is a period at the beginning of the stream
  // in which there will be no values pushed at all,
  // because it's checking whether the upstream source's initial value is stable.

  template <typename T, typename TTime, typename TInterval>
  struct debounce_push_handler {
    TInterval interval;
    push_fn<T> push;
    std::shared_ptr<Wrapper<bool>> did_pull;
    mutable std::optional<TaggedValue<T, TTime>> testing_new_state;
    mutable std::optional<T> current_state;
    mutable bool current_state_has_been_pushed;

    RHEO_NOINLINE void operator()(TaggedValue<T, TTime> value) const {
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

  template <typename T, typename TTime, typename TInterval>
  struct debounce_pull_handler {
    std::shared_ptr<Wrapper<bool>> did_pull;
    pull_fn inner_pull;

    RHEO_NOINLINE void operator()() const {
      did_pull->value = true;
      inner_pull();
      did_pull->value = false;
    }
  };

  template <typename T, typename TTime, typename TInterval>
  struct debounce_source_binder {
    source_fn<TaggedValue<T, TTime>> timestamped;
    TInterval interval;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      // We only want to push a value that's the same as the previously pushed value
      // if the push happens as a consequence of a pull.
      // It doesn't make sense for a push stream to force a debounced value to get emitted
      // for every single bouncy new value that's getting discarded.
      auto did_pull = make_wrapper_shared(false);

      pull_fn inner_pull = timestamped(debounce_push_handler<T, TTime, TInterval>{
        interval,
        std::move(push),
        did_pull,
        std::nullopt,
        std::nullopt,
        false
      });

      return debounce_pull_handler<T, TTime, TInterval>{did_pull, std::move(inner_pull)};
    }
  };

  template <typename T, typename TTime, typename TInterval>
  source_fn<T> debounce(source_fn<T> source, source_fn<TTime> clock_source, TInterval interval) {
    return debounce_source_binder<T, TTime, TInterval>{
      timestamp(source, clock_source),
      interval
    };
  }

  template <typename T, typename TTime, typename TInterval>
  pipe_fn<T, T> debounce(source_fn<TTime> clock_source, TInterval interval) {
    return [clock_source, interval](source_fn<T> source) {
      return debounce(source, clock_source, interval);
    };
  }

}