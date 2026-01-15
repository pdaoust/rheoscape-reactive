#pragma once

#include <cmath>
#include <optional>
#include <core_types.hpp>
#include <operators/scan.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>

namespace rheo {

  // Physical constants
  constexpr float WATER_SPECIFIC_HEAT_J_PER_KG_K = 4186.0f;
  constexpr float WATER_DENSITY_KG_PER_L = 1.0f;

  // Convert volume (liters) to thermal capacity (J/K)
  // Usage: water_volume_to_thermal_capacity(5.0f) -> 20930.0f J/K
  template <typename T>
  constexpr T water_volume_to_thermal_capacity(T volume_liters) {
    return volume_liters * static_cast<T>(WATER_DENSITY_KG_PER_L * WATER_SPECIFIC_HEAT_J_PER_KG_K);
  }

  // PWM configuration
  template <typename TTime>
  struct PwmConfig {
    TTime min_cycle_time;  // Minimum PWM cycle time (e.g., 1000ms for slow-switching SSR)
  };

  // Thermal simulation configuration
  template <typename TTemp, typename TPower, typename TTime>
  struct ThermalSimConfig {
    TTemp ambient_temperature;        // K or degrees C
    TPower max_heater_power;          // W
    TTemp thermal_capacity;           // J/K (use water_volume_to_thermal_capacity)
    TTemp heat_transfer_coefficient;  // W/K
    PwmConfig<TTime> pwm;             // PWM cycle configuration
  };

  // Internal state for thermal simulation
  template <typename TTemp, typename TTime, typename TDuty>
  struct ThermalSimState {
    TTemp temperature;
    TTime last_update;
    // PWM state
    TTime cycle_start;
    TDuty current_duty;
    bool heater_on;
  };

  // Combined input for thermal sim scanner
  template <typename TDuty, typename TTime, typename TPower>
  struct ThermalSimInput {
    TDuty duty;
    TTime timestamp;
    TPower disturbance;  // External heat injection (W), 0 if no disturbance
  };

  // Named callable for combining thermal sim inputs
  template <typename TDuty, typename TTime, typename TPower>
  struct thermal_sim_combiner {
    RHEO_NOINLINE ThermalSimInput<TDuty, TTime, TPower> operator()(
      TDuty duty,
      TTime timestamp,
      TPower disturbance
    ) const {
      return ThermalSimInput<TDuty, TTime, TPower>{ duty, timestamp, disturbance };
    }
  };

  // Named callable for combining thermal sim inputs (no disturbance)
  template <typename TDuty, typename TTime, typename TPower>
  struct thermal_sim_combiner_no_disturbance {
    RHEO_NOINLINE ThermalSimInput<TDuty, TTime, TPower> operator()(
      TDuty duty,
      TTime timestamp
    ) const {
      return ThermalSimInput<TDuty, TTime, TPower>{ duty, timestamp, TPower{0} };
    }
  };

  // Named callable for thermal simulation physics with PWM
  template <typename TTemp, typename TPower, typename TTime, typename TDuty>
  struct thermal_sim_scanner {
    ThermalSimConfig<TTemp, TPower, TTime> config;

    RHEO_NOINLINE ThermalSimState<TTemp, TTime, TDuty> operator()(
      ThermalSimState<TTemp, TTime, TDuty> state,
      ThermalSimInput<TDuty, TTime, TPower> input
    ) const {
      // Calculate time delta
      TTime dt = input.timestamp - state.last_update;
      float dt_seconds = static_cast<float>(dt);

      // Handle PWM: determine if heater is on or off
      TTime time_in_cycle = input.timestamp - state.cycle_start;
      TTime on_time = static_cast<TTime>(static_cast<float>(config.pwm.min_cycle_time) * static_cast<float>(input.duty));

      // Check if we need to start a new PWM cycle
      bool new_cycle = time_in_cycle >= config.pwm.min_cycle_time;
      TTime cycle_start = new_cycle ? input.timestamp : state.cycle_start;
      time_in_cycle = new_cycle ? TTime{0} : time_in_cycle;

      // Recalculate on_time for new duty if cycle restarted
      if (new_cycle) {
        on_time = static_cast<TTime>(static_cast<float>(config.pwm.min_cycle_time) * static_cast<float>(input.duty));
      }

      // Heater is on if we're within the on portion of the cycle
      bool heater_on = time_in_cycle < on_time;

      // Calculate heat flows
      // Q_in = heater_power when on, 0 when off (PWM creates sawtooth)
      TPower q_in = heater_on ? config.max_heater_power : TPower{0};

      // Q_out = k * (T - T_ambient)
      TPower q_out = config.heat_transfer_coefficient * (state.temperature - config.ambient_temperature);

      // Q_disturbance from external source
      TPower q_disturbance = input.disturbance;

      // Temperature change: dT = (Q_in - Q_out + Q_disturbance) * dt / C
      TTemp dT = (q_in - q_out + q_disturbance) * dt_seconds / config.thermal_capacity;

      return ThermalSimState<TTemp, TTime, TDuty>{
        state.temperature + dT,
        input.timestamp,
        cycle_start,
        input.duty,
        heater_on
      };
    }
  };

  // Named callable for extracting temperature from state
  template <typename TTemp, typename TTime, typename TDuty>
  struct thermal_sim_temp_extractor {
    RHEO_NOINLINE TTemp operator()(ThermalSimState<TTemp, TTime, TDuty> state) const {
      return state.temperature;
    }
  };

  // Thermal simulation source (with disturbance)
  // Models a simple first-order thermal system with PWM control
  //
  // Physics: C * dT/dt = Q_in - Q_out + Q_disturbance
  //   Q_in = heater_power when PWM on, 0 when off
  //   Q_out = k * (T - T_ambient)
  //
  // PWM creates sawtooth temperature ripple, which is important for
  // testing autotuning robustness.
  template <typename TTemp, typename TTime, typename TDuty, typename TPower>
  source_fn<TTemp> thermal_sim(
    source_fn<TDuty> duty_source,
    source_fn<TTime> clock_source,
    source_fn<TPower> disturbance_source,
    ThermalSimConfig<TTemp, TPower, TTime> config,
    TTemp initial_temperature
  ) {
    using InputType = ThermalSimInput<TDuty, TTime, TPower>;
    using StateType = ThermalSimState<TTemp, TTime, TDuty>;

    source_fn<InputType> combined_source = operators::combine(
      thermal_sim_combiner<TDuty, TTime, TPower>{},
      duty_source,
      clock_source,
      disturbance_source
    );

    StateType initial_state{
      initial_temperature,
      TTime{},      // Will be set on first update
      TTime{},      // PWM cycle start
      TDuty{0},     // Initial duty
      false         // Heater off
    };

    source_fn<StateType> state_source = operators::scan(
      combined_source,
      initial_state,
      thermal_sim_scanner<TTemp, TPower, TTime, TDuty>{config}
    );

    return operators::map(state_source, thermal_sim_temp_extractor<TTemp, TTime, TDuty>{});
  }

  // Thermal simulation source (no disturbance)
  // Simplified version when external disturbances are not needed
  template <typename TTemp, typename TTime, typename TDuty, typename TPower>
  source_fn<TTemp> thermal_sim(
    source_fn<TDuty> duty_source,
    source_fn<TTime> clock_source,
    ThermalSimConfig<TTemp, TPower, TTime> config,
    TTemp initial_temperature
  ) {
    using InputType = ThermalSimInput<TDuty, TTime, TPower>;
    using StateType = ThermalSimState<TTemp, TTime, TDuty>;

    source_fn<InputType> combined_source = operators::combine(
      thermal_sim_combiner_no_disturbance<TDuty, TTime, TPower>{},
      duty_source,
      clock_source
    );

    StateType initial_state{
      initial_temperature,
      TTime{},      // Will be set on first update
      TTime{},      // PWM cycle start
      TDuty{0},     // Initial duty
      false         // Heater off
    };

    source_fn<StateType> state_source = operators::scan(
      combined_source,
      initial_state,
      thermal_sim_scanner<TTemp, TPower, TTime, TDuty>{config}
    );

    return operators::map(state_source, thermal_sim_temp_extractor<TTemp, TTime, TDuty>{});
  }

  // Helper to create a typical sous vide configuration
  template <typename TTemp = float, typename TPower = float, typename TTime = unsigned long>
  ThermalSimConfig<TTemp, TPower, TTime> make_sous_vide_config(
    TTemp volume_liters,
    TPower heater_watts,
    TTemp ambient_temp = 20.0f,
    TTemp heat_loss_coefficient = 5.0f,  // W/K, typical for insulated container
    TTime pwm_cycle_ms = 1000             // 1 second PWM cycle
  ) {
    return ThermalSimConfig<TTemp, TPower, TTime>{
      ambient_temp,
      heater_watts,
      water_volume_to_thermal_capacity(volume_liters),
      heat_loss_coefficient,
      PwmConfig<TTime>{ pwm_cycle_ms }
    };
  }

}
