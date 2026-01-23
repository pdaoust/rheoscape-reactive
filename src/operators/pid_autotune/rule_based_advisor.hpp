#pragma once

#include <cmath>
#include <optional>
#include <core_types.hpp>
#include <operators/scan.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>
#include <operators/pid.hpp>
#include <operators/pid_autotune/autotune_types.hpp>

namespace rheo::autotune {

  // Extended configuration for rule-based advisor with cooldown support.
  template <typename TP, typename TTimePoint, typename TFitness = float>
  struct RuleBasedAdvisorConfig {
    TP overshoot_threshold;         // Trigger if overshoot exceeds this
    TP oscillation_amplitude;       // Trigger if oscillating more than this
    TTimePoint settling_window;          // Time window for settling detection
    TP settling_tolerance;          // Error tolerance for "settled"
    float adjustment_factor;        // How much to adjust weights (e.g., 0.1 = 10%)
    TTimePoint min_adjustment_interval;  // Don't adjust more often than this
    TFitness target_fitness;        // Cool down when fitness <= this (lower is better)
  };

  // Internal state for rule-based advisor.
  template <typename TP, typename TTimePoint, typename TFitness = float>
  struct RuleBasedState {
    // Performance tracking
    TP peak_error;                  // Maximum positive error seen
    TP valley_error;                // Maximum negative error seen
    TP accumulated_error;           // For steady-state error calculation
    int sample_count;               // Number of samples
    int zero_crossing_count;        // For oscillation detection
    bool last_error_positive;       // For zero crossing detection
    TTimePoint first_sample_time;        // When measurement started
    TTimePoint last_adjustment_time;     // When we last made an adjustment

    // Fitness tracking
    TFitness current_fitness;       // Computed from performance metrics
    bool is_cooled_down;            // True when fitness <= target

    // State flags
    bool first_sample;              // First sample flag
    TTimePoint settling_start_time;      // When error entered tolerance band
    bool in_tolerance_band;         // Currently within settling tolerance
  };

  // Combined input for rule-based advisor.
  template <typename TCtrl, typename TP, typename TI, typename TD, typename TTimePoint, typename TFitness>
  struct RuleBasedInput {
    operators::PidOutput<TCtrl, TP, TI> pid_output;
    TTimePoint timestamp;
    TFitness target_fitness;  // Dynamic target from source
  };

  // Named callable for combining advisor inputs.
  template <typename TCtrl, typename TP, typename TI, typename TD, typename TTimePoint, typename TFitness>
  struct rule_based_input_combiner {
    RHEO_NOINLINE RuleBasedInput<TCtrl, TP, TI, TD, TTimePoint, TFitness> operator()(
      operators::PidOutput<TCtrl, TP, TI> pid_output,
      TTimePoint timestamp,
      TFitness target_fitness
    ) const {
      return RuleBasedInput<TCtrl, TP, TI, TD, TTimePoint, TFitness>{
        pid_output,
        timestamp,
        target_fitness
      };
    }
  };

  // Named callable for combining advisor inputs (scalar target).
  template <typename TCtrl, typename TP, typename TI, typename TD, typename TTimePoint, typename TFitness>
  struct rule_based_input_combiner_scalar_target {
    TFitness target_fitness;

    RHEO_NOINLINE RuleBasedInput<TCtrl, TP, TI, TD, TTimePoint, TFitness> operator()(
      operators::PidOutput<TCtrl, TP, TI> pid_output,
      TTimePoint timestamp
    ) const {
      return RuleBasedInput<TCtrl, TP, TI, TD, TTimePoint, TFitness>{
        pid_output,
        timestamp,
        target_fitness
      };
    }
  };

  // Compute fitness score from current state.
  // Lower is better (matches quality_score semantics).
  template <typename TP, typename TTimePoint, typename TFitness>
  TFitness compute_fitness_from_state(const RuleBasedState<TP, TTimePoint, TFitness>& state) {
    if (state.sample_count == 0) {
      return std::numeric_limits<TFitness>::max();
    }

    // Compute metrics
    float overshoot = std::abs(static_cast<float>(state.peak_error));
    float undershoot = std::abs(static_cast<float>(state.valley_error));
    float steady_state_error = std::abs(
      static_cast<float>(state.accumulated_error) / static_cast<float>(state.sample_count)
    );
    float oscillations = static_cast<float>(state.zero_crossing_count);

    // Weighted combination (lower is better)
    return static_cast<TFitness>(
      overshoot * 0.3f +
      undershoot * 0.1f +
      steady_state_error * 0.3f +
      oscillations * 0.3f
    );
  }

  // Named callable for rule-based advisor scanner.
  template <typename TCtrl, typename TP, typename TI, typename TD, typename TTimePoint, typename TFitness>
  struct rule_based_scanner {
    RuleBasedAdvisorConfig<TP, TTimePoint, TFitness> config;

    using StateType = RuleBasedState<TP, TTimePoint, TFitness>;
    using InputType = RuleBasedInput<TCtrl, TP, TI, TD, TTimePoint, TFitness>;

    RHEO_NOINLINE StateType operator()(StateType state, InputType input) const {
      // Handle first sample
      if (state.first_sample) {
        TP error = input.pid_output.error;
        return StateType{
          error,                      // peak_error
          error,                      // valley_error
          error,                      // accumulated_error
          1,                          // sample_count
          0,                          // zero_crossing_count
          error > TP{0},              // last_error_positive
          input.timestamp,            // first_sample_time
          input.timestamp,            // last_adjustment_time
          std::numeric_limits<TFitness>::max(),  // current_fitness (unknown yet)
          false,                      // is_cooled_down
          false,                      // first_sample
          input.timestamp,            // settling_start_time
          std::abs(static_cast<float>(error)) <= static_cast<float>(config.settling_tolerance)
        };
      }

      auto result = state;
      TP error = input.pid_output.error;

      // Track peak and valley errors
      if (error > result.peak_error) {
        result.peak_error = error;
      }
      if (error < result.valley_error) {
        result.valley_error = error;
      }

      // Accumulate error for steady-state calculation
      result.accumulated_error = result.accumulated_error + error;
      result.sample_count = result.sample_count + 1;

      // Detect zero crossings for oscillation counting
      bool error_positive = error > TP{0};
      if (error_positive != result.last_error_positive) {
        result.zero_crossing_count = result.zero_crossing_count + 1;
        result.last_error_positive = error_positive;
      }

      // Track settling
      bool in_tolerance = std::abs(static_cast<float>(error)) <=
                          static_cast<float>(config.settling_tolerance);
      if (in_tolerance && !result.in_tolerance_band) {
        result.settling_start_time = input.timestamp;
      }
      result.in_tolerance_band = in_tolerance;

      // Compute current fitness
      result.current_fitness = compute_fitness_from_state(result);

      // Check for cooldown
      result.is_cooled_down = result.current_fitness <= input.target_fitness;

      return result;
    }
  };

  // Determine tuning adjustment based on current performance.
  template <typename TP, typename TTimePoint, typename TFitness>
  TuningAdjustment determine_adjustment(
    const RuleBasedState<TP, TTimePoint, TFitness>& state,
    const RuleBasedAdvisorConfig<TP, TTimePoint, TFitness>& config,
    TTimePoint current_time
  ) {
    // If cooled down (target fitness achieved), no adjustment needed
    if (state.is_cooled_down) {
      return TuningAdjustment::none;
    }

    // Respect minimum adjustment interval
    TTimePoint since_last = current_time - state.last_adjustment_time;
    if (since_last < config.min_adjustment_interval) {
      return TuningAdjustment::none;
    }

    // Need enough samples for meaningful metrics
    if (state.sample_count < 10) {
      return TuningAdjustment::none;
    }

    // Calculate metrics
    float overshoot = std::abs(static_cast<float>(state.peak_error));
    float undershoot = std::abs(static_cast<float>(state.valley_error));
    float amplitude = overshoot + undershoot;
    float steady_state_error = std::abs(
      static_cast<float>(state.accumulated_error) / static_cast<float>(state.sample_count)
    );

    // Check for sustained oscillation (many zero crossings)
    bool is_oscillating = state.zero_crossing_count > 6;

    // Heuristic rules (priority order)

    // Rule 1: Oscillation with overshoot -> aggressive Kp decrease
    if (is_oscillating && overshoot > static_cast<float>(config.overshoot_threshold)) {
      return TuningAdjustment::decrease_kp;
    }

    // Rule 2: Sustained oscillation -> decrease Kp or increase Kd
    if (is_oscillating && amplitude > static_cast<float>(config.oscillation_amplitude)) {
      return TuningAdjustment::decrease_kp;
    }

    // Rule 3: Overshoot without oscillation -> increase Kd
    if (overshoot > static_cast<float>(config.overshoot_threshold) && !is_oscillating) {
      return TuningAdjustment::increase_kd;
    }

    // Rule 4: Large steady-state error -> increase Ki
    if (steady_state_error > static_cast<float>(config.settling_tolerance) * 2.0f) {
      return TuningAdjustment::increase_ki;
    }

    // Rule 5: Slow response (not settled within window) -> increase Kp
    // (This would require tracking settling time, simplified here)

    return TuningAdjustment::none;
  }

  // Named callable for extracting TuningAdjustment from state.
  template <typename TP, typename TTimePoint, typename TFitness>
  struct rule_based_output_mapper {
    RuleBasedAdvisorConfig<TP, TTimePoint, TFitness> config;

    // We need the current timestamp, which we can get from the state's last update
    // This is a bit awkward; ideally we'd have timestamp in the mapper call
    mutable TTimePoint last_timestamp;

    RHEO_NOINLINE TuningAdjustment operator()(
      RuleBasedState<TP, TTimePoint, TFitness> state
    ) const {
      // Use stored timestamp (this is a simplification; proper implementation
      // would thread the timestamp through)
      TTimePoint current_time = state.first_sample_time;  // Approximate
      if (state.sample_count > 0) {
        // Estimate based on sample count (rough approximation)
        current_time = state.settling_start_time;
      }

      return determine_adjustment(state, config, current_time);
    }
  };

  // Combined state and timestamp for proper output mapping
  template <typename TP, typename TTimePoint, typename TFitness>
  struct RuleBasedStateWithTime {
    RuleBasedState<TP, TTimePoint, TFitness> state;
    TTimePoint timestamp;
  };

  // Scanner that preserves timestamp
  template <typename TCtrl, typename TP, typename TI, typename TD, typename TTimePoint, typename TFitness>
  struct rule_based_scanner_with_time {
    RuleBasedAdvisorConfig<TP, TTimePoint, TFitness> config;
    rule_based_scanner<TCtrl, TP, TI, TD, TTimePoint, TFitness> inner_scanner;

    using StateType = RuleBasedStateWithTime<TP, TTimePoint, TFitness>;
    using InputType = RuleBasedInput<TCtrl, TP, TI, TD, TTimePoint, TFitness>;

    rule_based_scanner_with_time(RuleBasedAdvisorConfig<TP, TTimePoint, TFitness> cfg)
      : config(cfg), inner_scanner{cfg} {}

    RHEO_NOINLINE StateType operator()(StateType state, InputType input) const {
      return StateType{
        inner_scanner(state.state, input),
        input.timestamp
      };
    }
  };

  // Output mapper with proper timestamp access
  template <typename TP, typename TTimePoint, typename TFitness>
  struct rule_based_output_mapper_with_time {
    RuleBasedAdvisorConfig<TP, TTimePoint, TFitness> config;

    RHEO_NOINLINE TuningAdjustment operator()(
      RuleBasedStateWithTime<TP, TTimePoint, TFitness> state_with_time
    ) const {
      return determine_adjustment(state_with_time.state, config, state_with_time.timestamp);
    }
  };

  // Rule-based advisor source function factory (with dynamic target fitness).
  //
  // Monitors PID output and recommends tuning adjustments based on heuristic rules.
  // Cools down when fitness reaches target to avoid over-tuning.
  //
  // Parameters:
  //   pid_output_source: Source providing PidOutput from pid_detailed()
  //   clock_source: Source providing timestamps
  //   target_fitness_source: Source providing dynamic target fitness threshold
  //   config: Advisor configuration
  //
  // Returns:
  //   source_fn<TuningAdjustment> - Recommended adjustment (or none if cooled down)
  template <
    typename TCtrl,
    typename TP,
    typename TI,
    typename TD,
    typename TTimePoint,
    typename TFitness = float
  >
  source_fn<TuningAdjustment> rule_based_advisor(
    source_fn<operators::PidOutput<TCtrl, TP, TI>> pid_output_source,
    source_fn<TTimePoint> clock_source,
    source_fn<TFitness> target_fitness_source,
    RuleBasedAdvisorConfig<TP, TTimePoint, TFitness> config
  ) {
    using InputType = RuleBasedInput<TCtrl, TP, TI, TD, TTimePoint, TFitness>;
    using StateType = RuleBasedStateWithTime<TP, TTimePoint, TFitness>;

    // Combine input sources
    source_fn<InputType> combined_source = operators::combine(
      rule_based_input_combiner<TCtrl, TP, TI, TD, TTimePoint, TFitness>{},
      pid_output_source,
      clock_source,
      target_fitness_source
    );

    // Initial state
    RuleBasedState<TP, TTimePoint, TFitness> initial_inner_state{
      TP{0},        // peak_error
      TP{0},        // valley_error
      TP{0},        // accumulated_error
      0,            // sample_count
      0,            // zero_crossing_count
      false,        // last_error_positive
      TTimePoint{0},     // first_sample_time
      TTimePoint{0},     // last_adjustment_time
      std::numeric_limits<TFitness>::max(),  // current_fitness
      false,        // is_cooled_down
      true,         // first_sample
      TTimePoint{0},     // settling_start_time
      false         // in_tolerance_band
    };

    StateType initial_state{initial_inner_state, TTimePoint{0}};

    // Scan to accumulate state
    source_fn<StateType> state_source = operators::scan(
      combined_source,
      initial_state,
      rule_based_scanner_with_time<TCtrl, TP, TI, TD, TTimePoint, TFitness>{config}
    );

    // Map to adjustment output
    return operators::map(
      state_source,
      rule_based_output_mapper_with_time<TP, TTimePoint, TFitness>{config}
    );
  }

  // Rule-based advisor source function factory (with scalar target fitness).
  //
  // Simplified version with fixed target fitness threshold.
  template <
    typename TCtrl,
    typename TP,
    typename TI,
    typename TD,
    typename TTimePoint,
    typename TFitness = float
  >
  source_fn<TuningAdjustment> rule_based_advisor(
    source_fn<operators::PidOutput<TCtrl, TP, TI>> pid_output_source,
    source_fn<TTimePoint> clock_source,
    RuleBasedAdvisorConfig<TP, TTimePoint, TFitness> config
  ) {
    using InputType = RuleBasedInput<TCtrl, TP, TI, TD, TTimePoint, TFitness>;
    using StateType = RuleBasedStateWithTime<TP, TTimePoint, TFitness>;

    // Combine input sources (with scalar target)
    source_fn<InputType> combined_source = operators::combine(
      rule_based_input_combiner_scalar_target<TCtrl, TP, TI, TD, TTimePoint, TFitness>{config.target_fitness},
      pid_output_source,
      clock_source
    );

    // Initial state
    RuleBasedState<TP, TTimePoint, TFitness> initial_inner_state{
      TP{0},        // peak_error
      TP{0},        // valley_error
      TP{0},        // accumulated_error
      0,            // sample_count
      0,            // zero_crossing_count
      false,        // last_error_positive
      TTimePoint{0},     // first_sample_time
      TTimePoint{0},     // last_adjustment_time
      std::numeric_limits<TFitness>::max(),  // current_fitness
      false,        // is_cooled_down
      true,         // first_sample
      TTimePoint{0},     // settling_start_time
      false         // in_tolerance_band
    };

    StateType initial_state{initial_inner_state, TTimePoint{0}};

    // Scan to accumulate state
    source_fn<StateType> state_source = operators::scan(
      combined_source,
      initial_state,
      rule_based_scanner_with_time<TCtrl, TP, TI, TD, TTimePoint, TFitness>{config}
    );

    // Map to adjustment output
    return operators::map(
      state_source,
      rule_based_output_mapper_with_time<TP, TTimePoint, TFitness>{config}
    );
  }

}
