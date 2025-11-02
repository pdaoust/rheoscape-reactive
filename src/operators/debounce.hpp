#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/timestamp.hpp>

namespace rheo::operators {
  
  // Don't emit a value until it's settled down
  // and matches the value recorded at the beginning of the interval.
  // Note: There is a period at the beginning of the stream
  // in which there will be no values pushed at all,
  // because it's checking whether the upstream source's initial value is stable.
  template <typename T, typename TTime, typename TInterval>
  source_fn<T> debounce(source_fn<T> source, source_fn<TTime> clockSource, TInterval interval) {
    auto timestamped = timestamp(source, clockSource);

    return [interval, timestamped](push_fn<T> push) {
      // We only want to push a value that's the same as the previously pushed value
      // if the push happens as a consequence of a pull.
      // It doesn't make sense for a push stream to force a debounced value to get emitted
      // for every single bouncy new value that's getting discarded.
      auto didPull = make_wrapper_shared(false);

      pull_fn pull = timestamped([
        interval,
        push,
        didPull,
        testingNewState = std::optional<TaggedValue<T, TTime>>(),
        currentState = std::optional<T>(),
        currentStateHasBeenPushed = false
      ](TaggedValue<T, TTime> value) mutable {
        if (testingNewState.has_value()
            // NOTE: This equation has to be this way
            // to guard against wraparound for unsigned time representations.
            // https://arduino.stackexchange.com/a/12588
            && value.tag - testingNewState.value().tag >= interval) {
          // The debounce period has settled down.
          // Has it held the new state value to the end of the settling period?
          if (value.value == testingNewState.value().value) {
            // Yup, this is now the new state.
            currentState = std::optional(value.value);
          }
          // Reset the testing value in preparation for the next state change.
          testingNewState = std::nullopt;
        }

        // If the new value isn't the same as the current state,
        // or if there is no current or testing states yet,
        // start a new debounce test period.
        if (
          (!currentState.has_value() || currentState.value() != value.value)
          && !testingNewState.has_value()
        ) {
          testingNewState = std::optional(value);
        }

        // Guard against initial non-state.
        if (currentState.has_value() && (!currentStateHasBeenPushed || didPull->value)) {
          // Push the current, not-bouncy state, whatever it is.
          push(currentState.value());
          currentStateHasBeenPushed = true;
        }
      });

      return [didPull, pull]() {
        didPull->value = true;
        pull();
        didPull->value = false;
      };
    };
  }

  template <typename T, typename TTime, typename TInterval>
  pipe_fn<T, T> debounce(source_fn<TTime> clockSource, TInterval interval) {
    return [clockSource, interval](source_fn<T> source) {
      return debounce(source, clockSource, interval);
    };
  }

}