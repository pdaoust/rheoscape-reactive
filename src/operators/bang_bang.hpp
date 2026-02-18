#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <types/Range.hpp>
#include <operators/combine.hpp>
#include <operators/scan.hpp>

namespace rheoscape::operators {

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
  template <typename PVSourceT, typename BoundsSourceT>
    requires concepts::Source<PVSourceT> && concepts::Source<BoundsSourceT>
  auto bang_bang(
    PVSourceT process_variable_source,
    BoundsSourceT bounds_source
  ) {
    using T = source_value_t<PVSourceT>;

    struct Scanner {
      RHEOSCAPE_CALLABLE ProcessCommand operator()(ProcessCommand acc, std::tuple<T, Range<T>> value) const {
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
    };

    auto combined = combine(std::move(process_variable_source), std::move(bounds_source));
    return scan(std::move(combined), ProcessCommand::neutral, Scanner{});
  }

  namespace detail {
    template <typename BoundsSourceT>
    struct BangBangPipeFactory {
      BoundsSourceT bounds_source;

      template <typename PVSourceT>
        requires concepts::Source<PVSourceT>
      RHEOSCAPE_CALLABLE auto operator()(PVSourceT process_variable_source) const {
        return bang_bang(std::move(process_variable_source), BoundsSourceT(bounds_source));
      }
    };
  }

  template <typename BoundsSourceT>
    requires concepts::Source<BoundsSourceT>
  auto bang_bang(BoundsSourceT bounds_source) {
    return detail::BangBangPipeFactory<BoundsSourceT>{std::move(bounds_source)};
  }

  // Drive a heater, cooler, sprinkler valve, etc.
  // Takes a process command value that tells it what direction 'on' means.
  // E.g., a heater drives temperature upwards when it turns on,
  // so on_drives_process should be ProcessCommand::up.
  map_fn<SwitchState, ProcessCommand> drive_plant(ProcessCommand on_drives_process) {
    struct Mapper {
      ProcessCommand on_drives_process;

      RHEOSCAPE_CALLABLE SwitchState operator()(ProcessCommand direction) const {
        return direction == on_drives_process ? SwitchState::on : SwitchState::off;
      }
    };

    return Mapper{on_drives_process};
  }

  // Drive a servo, linear actuator, etc.
  // Takes a process command value that tells it what direction 'open' means.
  // E.g., a vent drives temperature downwards when it opens,
  // so open_drives_process should be ProcessCommand::down.
  map_fn<GateCommand, ProcessCommand> drive_gate(ProcessCommand open_drives_process) {
    struct Mapper {
      ProcessCommand open_drives_process;

      RHEOSCAPE_CALLABLE GateCommand operator()(ProcessCommand direction) const {
        return direction == open_drives_process ? GateCommand::open : GateCommand::close;
      }
    };

    return Mapper{open_drives_process};
  }

}
