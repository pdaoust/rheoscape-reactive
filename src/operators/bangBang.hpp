#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Range.hpp>
#include <operators/fold.hpp>
#include <operators/zip.hpp>

namespace rheo {

  enum ProcessCommand {
    // Below target; push the process up.
    up,
    // Above target; push the process down.
    down,
    // This value is necessary in case the bang bang process starts inside the setpoint range.
    neutral,
  };

  // Control a process using a bang-bang algorithm,
  // attempting to keep the process variable within bounds.
  // If the process variable is already within bounds at the start,
  // no up or down values will be pushed until it goes out of bounds.
  // After that, it'll keep pushing in a direction until it passes the other bound,
  // just like a thermostat.
  template <typename T>
  source_fn<ProcessCommand> bangBang(source_fn<T> processVariableSource, source_fn<SetpointAndHysteresis<T>> boundsSource) {
    auto zipped = zip(
      processVariableSource,
      boundsSource
    );

    return fold(
      zipped,
      ProcessCommand::neutral,
      [](ProcessCommand acc, std::tuple<T, SetpointAndHysteresis<T>> value) {
        if (std::get<0>(value) < std::get<1>(value).min) {
          return ProcessCommand::up;
        } else if (std::get<0>(value) > std::get<1>(value).max) {
          return ProcessCommand::down;
        } else {
          // In the dead zone.
          // Keep on pushing it whatever direction it was previously going in
          // until it goes out of bounds.
          return acc;
        }
      }
    );
  }

  template <typename T>
  pipe_fn<T, T> bangBang(source_fn<SetpointAndHysteresis<T>> boundsSource) {
    return [boundsSource](source_fn<T> processVariableSource) {
      return bangBang(processVariableSource, boundsSource);
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

}