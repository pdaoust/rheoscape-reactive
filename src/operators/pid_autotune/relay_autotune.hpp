#pragma once

#include <cmath>
#include <optional>
#include <core_types.hpp>
#include <operators/scan.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>
#include <operators/pid_autotune/autotune_types.hpp>

namespace rheo::autotune {

  // Internal state for relay autotune scanner.
  // Tracks oscillation measurements and calculates ultimate gain/period.
  template <typename TCtl, typename TP, typename TTime>
  struct RelayAutotuneState {
    TCtl current_output;           // Current relay output (high or low)
    TP last_peak;                  // Last recorded peak value
    TP last_valley;                // Last recorded valley value
    TTime last_crossing_time;      // Time of last setpoint crossing
    int crossing_count;            // Number of zero crossings
    TP sum_amplitudes;             // Sum of oscillation amplitudes for averaging
    TTime sum_periods;             // Sum of oscillation periods for averaging
    int measurement_count;         // Number of complete oscillation measurements
    bool above_setpoint;           // Current position relative to setpoint
    AutotuneStatus status;         // Current autotuning status
    TTime start_time;              // When autotuning started
    bool first_sample;             // True until first sample is processed
  };

  // Combined input for relay autotune scanner.
  template <typename TP, typename TTime>
  struct RelayAutotuneInput {
    TP process_variable;
    TP setpoint;
    TTime timestamp;
  };

  // Named callable for combining relay autotune inputs.
  template <typename TP, typename TTime>
  struct relay_autotune_input_combiner {
    RHEO_NOINLINE RelayAutotuneInput<TP, TTime> operator()(
      TP process_variable,
      TP setpoint,
      TTime timestamp
    ) const {
      return RelayAutotuneInput<TP, TTime>{ process_variable, setpoint, timestamp };
    }
  };

  // Named callable for relay autotune state machine.
  // Implements Astrom-Hagglund relay feedback method for Ziegler-Nichols tuning.
  template <typename TCtl, typename TP, typename TTime>
  struct relay_autotune_scanner {
    RelayAutotuneConfig<TCtl, TP, TTime> config;

    RHEO_NOINLINE RelayAutotuneState<TCtl, TP, TTime> operator()(
      RelayAutotuneState<TCtl, TP, TTime> state,
      RelayAutotuneInput<TP, TTime> input
    ) const {
      // Handle first sample: initialize state
      if (state.first_sample) {
        bool above = input.process_variable > input.setpoint;
        return RelayAutotuneState<TCtl, TP, TTime>{
          above ? config.low_output : config.high_output,
          input.process_variable,  // Initial peak
          input.process_variable,  // Initial valley
          input.timestamp,         // Last crossing time
          0,                       // No crossings yet
          TP{0},                   // No amplitude sum
          TTime{0},                // No period sum
          0,                       // No measurements
          above,                   // Current position
          AutotuneStatus::running,
          input.timestamp,         // Start time
          false                    // No longer first sample
        };
      }

      // Check for timeout
      TTime elapsed = input.timestamp - state.start_time;
      if (elapsed > config.max_duration) {
        auto result = state;
        result.status = AutotuneStatus::failed;
        return result;
      }

      // Already converged or failed - just maintain state
      if (state.status == AutotuneStatus::converged ||
          state.status == AutotuneStatus::failed) {
        return state;
      }

      auto result = state;

      // Determine thresholds with hysteresis
      TP upper_threshold = input.setpoint + config.hysteresis;
      TP lower_threshold = input.setpoint - config.hysteresis;

      // Track peak and valley
      if (state.above_setpoint) {
        // Above setpoint, tracking peak
        if (input.process_variable > result.last_peak) {
          result.last_peak = input.process_variable;
        }
      } else {
        // Below setpoint, tracking valley
        if (input.process_variable < result.last_valley) {
          result.last_valley = input.process_variable;
        }
      }

      // Check for crossing through hysteresis band
      bool crossed_down = state.above_setpoint && input.process_variable < lower_threshold;
      bool crossed_up = !state.above_setpoint && input.process_variable > upper_threshold;

      if (crossed_down || crossed_up) {
        // Calculate period since last crossing
        TTime period = input.timestamp - state.last_crossing_time;

        // Only count if period is long enough (filters PWM ripple)
        if (period >= config.min_period && state.crossing_count > 0) {
          // Calculate amplitude from peak to valley
          TP amplitude = result.last_peak - result.last_valley;
          result.sum_amplitudes = result.sum_amplitudes + amplitude;
          result.sum_periods = result.sum_periods + period;
          result.measurement_count = result.measurement_count + 1;
        }

        // Update state for crossing
        result.crossing_count = result.crossing_count + 1;
        result.last_crossing_time = input.timestamp;

        if (crossed_down) {
          result.above_setpoint = false;
          result.current_output = config.high_output;
          result.last_valley = input.process_variable;  // Reset valley tracking
        } else {
          result.above_setpoint = true;
          result.current_output = config.low_output;
          result.last_peak = input.process_variable;  // Reset peak tracking
        }

        // Check if we have enough oscillations
        if (result.measurement_count >= config.min_oscillations) {
          result.status = AutotuneStatus::converged;
        }
      }

      return result;
    }
  };

  // Calculate Ziegler-Nichols parameters from oscillation measurements.
  // Returns nullopt if not enough data.
  template <typename TCtl, typename TP, typename TTime, typename TKp, typename TKi, typename TKd>
  std::optional<RelayAutotuneResult<TKp, TKi, TKd, TTime>> calculate_zn_parameters(
    const RelayAutotuneState<TCtl, TP, TTime>& state,
    const RelayAutotuneConfig<TCtl, TP, TTime>& config
  ) {
    if (state.status != AutotuneStatus::converged || state.measurement_count == 0) {
      return std::nullopt;
    }

    // Calculate average amplitude and period
    float avg_amplitude = static_cast<float>(state.sum_amplitudes) / static_cast<float>(state.measurement_count);
    float avg_period = static_cast<float>(state.sum_periods) / static_cast<float>(state.measurement_count);

    // Relay amplitude: d = (high - low) / 2
    float d = (static_cast<float>(config.high_output) - static_cast<float>(config.low_output)) / 2.0f;

    // Temperature amplitude: a = avg_amplitude / 2 (peak-to-peak to amplitude)
    float a = avg_amplitude / 2.0f;

    // Prevent division by zero
    if (a < 0.0001f) {
      return std::nullopt;
    }

    // Ultimate gain: Ku = (4 * d) / (π * a)
    constexpr float PI = 3.14159265358979323846f;
    float Ku = (4.0f * d) / (PI * a);

    // Ultimate period: Tu = average period
    float Tu = avg_period;

    // Apply Ziegler-Nichols rules based on configuration
    float Kp_factor, Ki_factor, Kd_factor;
    switch (config.rule) {
      case ZieglerNicholsRule::classic:
        // ~25% overshoot
        Kp_factor = 0.6f;
        Ki_factor = 1.2f;
        Kd_factor = 0.075f;
        break;
      case ZieglerNicholsRule::no_overshoot:
        // Conservative, minimal overshoot
        Kp_factor = 0.2f;
        Ki_factor = 0.4f;
        Kd_factor = 0.066f;
        break;
      case ZieglerNicholsRule::pessen_integral:
        // Aggressive, good for integral action
        Kp_factor = 0.7f;
        Ki_factor = 1.75f;
        Kd_factor = 0.105f;
        break;
    }

    // Calculate PID gains
    // Kp = factor * Ku
    // Ki = factor * Ku / Tu
    // Kd = factor * Ku * Tu
    TKp Kp = static_cast<TKp>(Kp_factor * Ku);
    TKi Ki = static_cast<TKi>(Ki_factor * Ku / Tu);
    TKd Kd = static_cast<TKd>(Kd_factor * Ku * Tu);

    return RelayAutotuneResult<TKp, TKi, TKd, TTime>{
      static_cast<TKp>(Ku),
      static_cast<TTime>(Tu),
      Kp,
      Ki,
      Kd,
      state.status
    };
  }

  // Named callable for extracting RelayAutotuneOutput from state.
  template <typename TCtl, typename TP, typename TTime, typename TKp, typename TKi, typename TKd>
  struct relay_autotune_output_mapper {
    RelayAutotuneConfig<TCtl, TP, TTime> config;

    using OutputType = RelayAutotuneOutput<TCtl, TKp, TKi, TKd, TTime>;

    RHEO_NOINLINE OutputType operator()(
      RelayAutotuneState<TCtl, TP, TTime> state
    ) const {
      std::optional<RelayAutotuneResult<TKp, TKi, TKd, TTime>> result = std::nullopt;

      if (state.status == AutotuneStatus::converged) {
        result = calculate_zn_parameters<TCtl, TP, TTime, TKp, TKi, TKd>(state, config);
      }

      return OutputType{
        state.current_output,
        result
      };
    }
  };

  // Relay autotune source function factory.
  //
  // Implements the Astrom-Hagglund relay feedback method for PID autotuning.
  // The output alternates between high_output and low_output based on process
  // variable crossing the setpoint with hysteresis.
  // After min_oscillations stable oscillations, calculates Ku and Tu,
  // then applies Ziegler-Nichols rules to determine PID gains.
  //
  // Usage:
  //   auto autotune = relay_autotune<float, float, unsigned long>(
  //     pv_source, setpoint_source, clock_source, config
  //   );
  //   autotune([](auto output) {
  //     // output.control: current relay output (use to drive plant)
  //     // output.result: optional, populated when autotuning complete
  //   });
  template <
    typename TP,
    typename TCtl,
    typename TTime,
    typename TKp = TCtl,
    typename TKi = TCtl,
    typename TKd = TCtl
  >
  source_fn<RelayAutotuneOutput<TCtl, TKp, TKi, TKd, TTime>> relay_autotune(
    source_fn<TP> process_variable_source,
    source_fn<TP> setpoint_source,
    source_fn<TTime> clock_source,
    RelayAutotuneConfig<TCtl, TP, TTime> config
  ) {
    using InputType = RelayAutotuneInput<TP, TTime>;
    using StateType = RelayAutotuneState<TCtl, TP, TTime>;
    using OutputType = RelayAutotuneOutput<TCtl, TKp, TKi, TKd, TTime>;

    // Combine input sources
    source_fn<InputType> combined_source = operators::combine(
      relay_autotune_input_combiner<TP, TTime>{},
      process_variable_source,
      setpoint_source,
      clock_source
    );

    // Initial state
    StateType initial_state{
      config.high_output,   // Start with high output
      TP{0},                // last_peak
      TP{0},                // last_valley
      TTime{0},             // last_crossing_time
      0,                    // crossing_count
      TP{0},                // sum_amplitudes
      TTime{0},             // sum_periods
      0,                    // measurement_count
      false,                // above_setpoint
      AutotuneStatus::idle,
      TTime{0},             // start_time
      true                  // first_sample
    };

    // Scan to accumulate state
    source_fn<StateType> state_source = operators::scan(
      combined_source,
      initial_state,
      relay_autotune_scanner<TCtl, TP, TTime>{config}
    );

    // Map to output type
    return operators::map(
      state_source,
      relay_autotune_output_mapper<TCtl, TP, TTime, TKp, TKi, TKd>{config}
    );
  }

  // Pipe version of relay_autotune.
  // Takes process_variable as the piped source.
  template <
    typename TP,
    typename TCtl,
    typename TTime,
    typename TKp = TCtl,
    typename TKi = TCtl,
    typename TKd = TCtl
  >
  pipe_fn<RelayAutotuneOutput<TCtl, TKp, TKi, TKd, TTime>, TP> relay_autotune(
    source_fn<TP> setpoint_source,
    source_fn<TTime> clock_source,
    RelayAutotuneConfig<TCtl, TP, TTime> config
  ) {
    return [setpoint_source, clock_source, config](source_fn<TP> process_variable_source) {
      return relay_autotune<TP, TCtl, TTime, TKp, TKi, TKd>(
        process_variable_source,
        setpoint_source,
        clock_source,
        config
      );
    };
  }

}
