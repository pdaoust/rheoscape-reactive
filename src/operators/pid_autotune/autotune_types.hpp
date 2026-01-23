#pragma once

#include <optional>
#include <cstdint>
#include <operators/pid.hpp>

namespace rheo::autotune {

  // Status of autotuning process
  enum class AutotuneStatus {
    idle,       // Not started
    running,    // Actively tuning
    converged,  // Successfully found parameters
    failed,     // Could not converge
    aborted     // Manually stopped
  };

  // Ziegler-Nichols tuning rule variants
  enum class ZieglerNicholsRule {
    classic,          // Kp=0.6Ku, Ki=1.2Ku/Tu, Kd=0.075KuTu (~25% overshoot)
    no_overshoot,     // Kp=0.2Ku, Ki=0.4Ku/Tu, Kd=0.066KuTu (conservative)
    pessen_integral   // Kp=0.7Ku, Ki=1.75Ku/Tu, Kd=0.105KuTu (aggressive)
  };

  // Result from relay autotuner
  template <typename TKp, typename TKi, typename TKd, typename TTimePoint>
  struct RelayAutotuneResult {
    TKp Ku;             // Ultimate gain
    TTimePoint Tu;           // Ultimate period
    TKp Kp;             // Calculated proportional gain
    TKi Ki;             // Calculated integral gain
    TKd Kd;             // Calculated derivative gain
    AutotuneStatus status;
  };

  // Output from relay autotune operator - combines control signal and result
  template <typename TCtl, typename TKp, typename TKi, typename TKd, typename TTimePoint>
  struct RelayAutotuneOutput {
    TCtl control;       // Current relay output (high/low)
    std::optional<RelayAutotuneResult<TKp, TKi, TKd, TTimePoint>> result;  // Populated when complete
  };

  // Configuration for relay autotuner
  template <typename TCtl, typename TP, typename TTimePoint>
  struct RelayAutotuneConfig {
    TCtl high_output;         // Output when below setpoint
    TCtl low_output;          // Output when above setpoint
    TP hysteresis;            // Dead band around setpoint
    int min_oscillations;     // Minimum oscillations before calculating (typically 3-5)
    TTimePoint max_duration;       // Timeout for autotuning
    TTimePoint min_period;         // Minimum expected period (filters out noise/PWM ripple)
    ZieglerNicholsRule rule;  // Which Z-N rule to use for calculation
  };

  // Performance metrics for rule-based tuner
  template <typename TP, typename TTimePoint>
  struct PerformanceMetrics {
    TP overshoot;             // Max excursion past setpoint (positive direction)
    TP undershoot;            // Max excursion below setpoint (negative direction)
    TTimePoint rise_time;          // Time from 10% to 90% of setpoint change
    TTimePoint settling_time;      // Time to stay within tolerance of setpoint
    TP steady_state_error;    // Average error after settling
    int oscillation_count;    // Number of zero crossings after initial rise
    bool is_oscillating;      // Detected sustained oscillation
  };

  // Tuning adjustment recommendation from rule-based advisor
  enum class TuningAdjustment {
    none,
    increase_kp,
    decrease_kp,
    increase_ki,
    decrease_ki,
    increase_kd,
    decrease_kd
  };

  // Configuration for rule-based advisor
  template <typename TP, typename TTimePoint>
  struct RuleBasedConfig {
    TP overshoot_threshold;         // Trigger if overshoot exceeds this
    TP oscillation_amplitude;       // Trigger if oscillating more than this
    TTimePoint settling_window;          // Time window for settling detection
    TP settling_tolerance;          // Error tolerance for "settled"
    float adjustment_factor;        // How much to adjust weights (e.g., 0.1 = 10%)
    TTimePoint min_adjustment_interval;  // Don't adjust more often than this
  };

  // Plant parameters for KNN interpolation
  // Generic version - users can specialize for their domain
  template <typename T>
  struct PlantParameters {
    T thermal_mass;               // J/K or similar
    T heat_transfer_coefficient;  // W/K or similar
    T ambient_temperature;        // K or degrees
    T max_power;                  // W
    // Optional additional parameters
    std::optional<T> volume;
    std::optional<T> surface_area;

    // Distance metric for KNN (Euclidean distance normalized by scale)
    T distance_to(const PlantParameters<T>& other) const {
      T d_mass = (thermal_mass - other.thermal_mass) / thermal_mass;
      T d_htc = (heat_transfer_coefficient - other.heat_transfer_coefficient) / heat_transfer_coefficient;
      T d_ambient = (ambient_temperature - other.ambient_temperature) / ambient_temperature;
      T d_power = (max_power - other.max_power) / max_power;
      return std::sqrt(d_mass * d_mass + d_htc * d_htc + d_ambient * d_ambient + d_power * d_power);
    }
  };

  // Tuning record for storage
  template <typename TPlantParams, typename TKp, typename TKi, typename TKd>
  struct TuningRecord {
    TPlantParams plant_params;
    operators::PidWeights<TKp, TKi, TKd> weights;
    float quality_score;      // Lower = better (cost function)
    uint32_t timestamp;       // For LRU eviction
    bool valid;               // Slot occupied?
  };

  // Input type for table_sink
  template <typename TPlantParams, typename TKp, typename TKi, typename TKd>
  struct TuningTableSinkInput {
    TPlantParams plant_params;
    operators::PidWeights<TKp, TKi, TKd> weights;
    float quality_score;
  };

  // Compute quality score from performance metrics
  // Lower score = better performance
  template <typename TP, typename TTimePoint>
  float compute_quality_score(const PerformanceMetrics<TP, TTimePoint>& metrics) {
    // Convert types to float for scoring
    // This assumes TP and TTimePoint can be cast to float or have .count() for durations
    float settling = static_cast<float>(metrics.settling_time);
    float overshoot = static_cast<float>(metrics.overshoot);
    float error = static_cast<float>(metrics.steady_state_error);
    float oscillations = static_cast<float>(metrics.oscillation_count);

    return settling * 0.3f
         + overshoot * 0.3f
         + std::abs(error) * 0.2f
         + oscillations * 0.2f;
  }

}
