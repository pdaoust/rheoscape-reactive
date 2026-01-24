#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Range.hpp>
#include <operators/combine.hpp>
#include <operators/scan.hpp>

namespace rheo::operators {

  enum ProcessCommand {
    // Below target; push the process up.
    up = 1,
    // Above target; push the process down.
    down = -1,
    // This value is necessary in case the bang bang process starts inside the setpoint range.
    neutral = 0,
  };

  // Control a process using a bang-bang algorithm,
  // attempting to keep the process variable within bounds.
  // If the process variable is already within bounds at the start,
  // no up or down values will be pushed until it goes out of bounds.
  // After that, it'll keep pushing in a direction until it passes the other bound,
  // just like a thermostat.
  template <typename T>
  source_fn<ProcessCommand> bang_bang(
    source_fn<T> process_variable_source,
    source_fn<Range<T>> bounds_source
  ) {
    auto combined = combine(process_variable_source, bounds_source);

    return scan(
      std::move(combined),
      ProcessCommand::neutral,
      [](ProcessCommand acc, std::tuple<T, Range<T>> value) {
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
  pipe_fn<T, T> bang_bang(source_fn<Range<T>> bounds_source) {
    return [bounds_source = std::move<source_fn<Range<T>>>(bounds_source)](source_fn<T> process_variable_source) {
      return bang_bang(std::move<source_fn<T>>(process_variable_source), std::move<source_fn<Range<T>>>(bounds_source));
    };
  }

  // Drive a heater, cooler, sprinkler valve, etc.
  // Takes a process command value that tells it what direction 'on' means.
  // E.g., a heater drives temperature upwards when it turns on,
  // so on_drives_process should be ProcessCommand::up.
  map_fn<SwitchState, ProcessCommand> drive_plant(ProcessCommand on_drives_process) {
    return [on_drives_process](ProcessCommand direction) {
      return direction == on_drives_process ? SwitchState::on : SwitchState::off;
    };
  }

  // Drive a servo, linear actuator, etc.
  // Takes a process command value that tells it what direction 'open' means.
  // E.g., a vent drives temperature downwards when it opens,
  // so open_drives_process should be ProcessCommand::down.
  map_fn<GateCommand, ProcessCommand> drive_gate(ProcessCommand open_drives_process) {
    return [open_drives_process](ProcessCommand direction) {
      return direction == open_drives_process ? GateCommand::open : GateCommand::close;
    };
  }

}