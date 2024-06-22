#ifndef RHEOSCAPE_BANG_BANG
#define RHEOSCAPE_BANG_BANG

#include <functional>
#include <core_types.hpp>
#include <types/Range.hpp>

enum ProcessCommand {
  // Below target; push the process up.
  up,
  // Above target; push the process down.
  down,
};

// Control a process using a bang-bang algorithm,
// attempting to keep the process variable within bounds.
// If the process variable is already within bounds at the start,
// no up or down values will be pushed until it goes out of bounds.
// After that, it'll keep pushing in a direction until it passes the other bound,
// just like a thermostat.
template <typename T>
source_fn<ProcessCommand> bangBang_(source_fn<T> processVariableSource, source_fn<SetpointAndHysteresis<T>> boundsSource) {
  return [processVariableSource, boundsSource](push_fn<ProcessCommand> push) {
    std::optional<SetpointAndHysteresis<T>> bounds;
    pull_fn pullBounds = boundsSource([&bounds](SetpointAndHysteresis<T> value) { bounds = value; });

    std::optional<ProcessCommand> lastNonNeutralDirection;
    pull_fn pullProcessVariable = processVariableSource([push, bounds, &lastNonNeutralDirection](T value) {
      if (value < bounds.min()) {
        lastNonNeutralDirection = ProcessCommand::up;
        push(ProcessCommand::up);
      } else if (value > bounds.max()) {
        lastNonNeutralDirection = ProcessCommand::down;
        push(ProcessCommand::down);
      } else if (lastNonNeutralDirection.has_value()) {
        // In the dead zone _and_ the process was previously being pushed up or down.
        // Keep on pushing it in that direction until it goes out of bounds.
        push(lastNonNeutralDirection.value());
      }
    });
  };
}

template <typename T>
pipe_fn<T, T> bangBang(source_fn<SetpointAndHysteresis<T>> boundsSource) {
  return [boundsSource](source_fn<T> processVariableSource) {
    return bangBang_(processVariableSource, boundsSource);
  };
}

// Drive a heater, cooler, sprinkler valve, etc.
// Takes a process command value that tells it what direction 'on' means.
// E.g., a heater drives temperature upwards when it turns on,
// so onDrivesProcess should be ProcessCommand::up.
map_fn<SwitchState, ProcessCommand> drivePlant(ProcessCommand onDrivesProcess) {
  return [onDrivesProcess](ProcessCommand direction) {
    return direction == onDrivesProcess ? SwitchState::on : SwitchState::off;
  };
}

// Drive a servo, linear actuator, etc.
// Takes a process command value that tells it what direction 'open' means.
// E.g., a vent drives temperature downwards when it opens,
// so openDrivesProcess should be ProcessCommand::down.
map_fn<GateCommand, ProcessCommand> driveGate(ProcessCommand openDrivesProcess) {
  return [openDrivesProcess](ProcessCommand direction) {
    return direction == openDrivesProcess ? GateCommand::open : GateCommand::close;
  };
}

#endif