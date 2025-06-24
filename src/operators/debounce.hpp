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
      return timestamped([
        interval,
        push,
        testingNewState = std::optional<TaggedValue<T, TTime>>(),
        currentState = std::optional<T>()
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
            currentState = value.value;
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
          testingNewState = value;
        }

        // Guard against initial non-state.
        if (currentState.has_value()) {
          // Push the current, not-bouncy state, whatever it is.
          push(currentState.value());
        }
      });
    };
  }

  template <typename T, typename TTime, typename TInterval>
  pipe_fn<T, T> debounce(source_fn<TTime> clockSource, TInterval interval) {
    return [clockSource, interval](source_fn<T> source) {
      return debounce(source, clockSource, interval);
    };
  }

}