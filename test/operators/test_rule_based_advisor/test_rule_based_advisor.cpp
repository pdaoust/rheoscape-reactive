#include <unity.h>
#include <cmath>
#include <operators/pid_autotune/rule_based_advisor.hpp>
#include <operators/pid.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <sources/constant.hpp>

using namespace rheo;
using namespace rheo::autotune;
using namespace rheo::operators;
using namespace rheo::sources;

using clock_type = mock_clock_ulong_millis;
using time_point = clock_type::time_point;

// Helper source that returns raw milliseconds from mock_clock.
source_fn<unsigned long> raw_clock_source() {
  return [](push_fn<unsigned long> push) -> pull_fn {
    return [push]() {
      push(clock_type::now().time_since_epoch().count());
    };
  };
}

// Helper to create a PID output with specified error
// PidOutput fields: control, p_term, i_term, d_term, error, integral, is_saturated
PidOutput<float, float, float> make_pid_output(float error, float p_term = 0, float i_term = 0) {
  return PidOutput<float, float, float>{
    0.0f,     // control
    p_term,   // p_term
    i_term,   // i_term
    0.0f,     // d_term
    error,    // error (5th field!)
    0.0f,     // integral
    false     // is_saturated
  };
}

void test_advisor_detects_oscillation() {
  // Test that advisor recommends decrease_kp when oscillating.
  clock_type::set_time(1000);

  RuleBasedAdvisorConfig<float, unsigned long, float> config{
    2.0f,       // overshoot_threshold
    1.0f,       // oscillation_amplitude
    5000UL,     // settling_window
    0.5f,       // settling_tolerance
    0.1f,       // adjustment_factor
    1000UL,     // min_adjustment_interval
    0.1f        // target_fitness (low = good, we won't reach this)
  };

  State<PidOutput<float, float, float>> pid_output_state(make_pid_output(0.0f));

  auto advisor_source = rule_based_advisor<float, float, float, float, unsigned long, float>(
    pid_output_state.get_source_fn(),
    raw_clock_source(),
    config
  );

  TuningAdjustment adjustment = TuningAdjustment::none;
  pull_fn pull = advisor_source([&adjustment](auto v) { adjustment = v; });

  // Initial pull
  pull();

  // Simulate oscillations (many zero crossings with significant amplitude)
  // Error alternates: +3, -3, +3, -3, etc.
  for (int i = 0; i < 20; i++) {
    clock_type::tick(200);
    float error = (i % 2 == 0) ? 3.0f : -3.0f;
    pid_output_state.set(make_pid_output(error), false);
    pull();
  }

  // After many zero crossings with amplitude > oscillation_amplitude,
  // should recommend decreasing Kp
  TEST_ASSERT_EQUAL_MESSAGE(TuningAdjustment::decrease_kp, adjustment,
    "Should recommend decrease_kp for sustained oscillation");
}

void test_advisor_detects_overshoot() {
  // Test that advisor recommends increase_kd when overshoot occurs without oscillation.
  clock_type::set_time(1000);

  RuleBasedAdvisorConfig<float, unsigned long, float> config{
    2.0f,       // overshoot_threshold
    5.0f,       // oscillation_amplitude (high to avoid oscillation detection)
    5000UL,     // settling_window
    0.5f,       // settling_tolerance
    0.1f,       // adjustment_factor
    1000UL,     // min_adjustment_interval
    0.1f        // target_fitness
  };

  State<PidOutput<float, float, float>> pid_output_state(make_pid_output(0.0f));

  auto advisor_source = rule_based_advisor<float, float, float, float, unsigned long, float>(
    pid_output_state.get_source_fn(),
    raw_clock_source(),
    config
  );

  TuningAdjustment adjustment = TuningAdjustment::none;
  pull_fn pull = advisor_source([&adjustment](auto v) { adjustment = v; });

  // Initial pull
  pull();

  // Simulate overshoot without oscillation: large positive error, then settling
  // Start with large positive error (overshoot)
  for (int i = 0; i < 15; i++) {
    clock_type::tick(200);
    // Gradually decreasing overshoot, minimal zero crossings
    float error = 5.0f - (i * 0.3f);
    if (error < 0.5f) error = 0.5f;
    pid_output_state.set(make_pid_output(error), false);
    pull();
  }

  // Should recommend increase_kd for overshoot without oscillation
  TEST_ASSERT_EQUAL_MESSAGE(TuningAdjustment::increase_kd, adjustment,
    "Should recommend increase_kd for overshoot without oscillation");
}

void test_advisor_detects_steady_state_error() {
  // Test that advisor recommends increase_ki for large steady-state error.
  clock_type::set_time(1000);

  RuleBasedAdvisorConfig<float, unsigned long, float> config{
    5.0f,       // overshoot_threshold (high)
    5.0f,       // oscillation_amplitude (high)
    5000UL,     // settling_window
    0.5f,       // settling_tolerance
    0.1f,       // adjustment_factor
    1000UL,     // min_adjustment_interval
    0.1f        // target_fitness
  };

  State<PidOutput<float, float, float>> pid_output_state(make_pid_output(0.0f));

  auto advisor_source = rule_based_advisor<float, float, float, float, unsigned long, float>(
    pid_output_state.get_source_fn(),
    raw_clock_source(),
    config
  );

  TuningAdjustment adjustment = TuningAdjustment::none;
  pull_fn pull = advisor_source([&adjustment](auto v) { adjustment = v; });

  // Initial pull
  pull();

  // Simulate steady-state error: constant error above tolerance
  for (int i = 0; i < 15; i++) {
    clock_type::tick(200);
    // Constant error of 2.0 (above 2 * settling_tolerance = 1.0)
    pid_output_state.set(make_pid_output(2.0f), false);
    pull();
  }

  // Should recommend increase_ki for steady-state error
  TEST_ASSERT_EQUAL_MESSAGE(TuningAdjustment::increase_ki, adjustment,
    "Should recommend increase_ki for large steady-state error");
}

void test_advisor_respects_min_adjustment_interval() {
  // Test that advisor doesn't recommend adjustments too frequently.
  clock_type::set_time(1000);

  RuleBasedAdvisorConfig<float, unsigned long, float> config{
    1.0f,       // overshoot_threshold (low)
    1.0f,       // oscillation_amplitude (low)
    5000UL,     // settling_window
    0.5f,       // settling_tolerance
    0.1f,       // adjustment_factor
    5000UL,     // min_adjustment_interval = 5s
    0.1f        // target_fitness
  };

  State<PidOutput<float, float, float>> pid_output_state(make_pid_output(0.0f));

  auto advisor_source = rule_based_advisor<float, float, float, float, unsigned long, float>(
    pid_output_state.get_source_fn(),
    raw_clock_source(),
    config
  );

  TuningAdjustment adjustment = TuningAdjustment::none;
  pull_fn pull = advisor_source([&adjustment](auto v) { adjustment = v; });

  // Initial pull
  pull();

  // Create oscillation condition but with short time intervals
  for (int i = 0; i < 15; i++) {
    clock_type::tick(100);  // Only 100ms per iteration, < 5s total
    float error = (i % 2 == 0) ? 3.0f : -3.0f;
    pid_output_state.set(make_pid_output(error), false);
    pull();
  }

  // Should be none because we haven't exceeded min_adjustment_interval
  TEST_ASSERT_EQUAL_MESSAGE(TuningAdjustment::none, adjustment,
    "Should not recommend adjustment before min_adjustment_interval");
}

void test_advisor_cools_down_at_target_fitness() {
  // Test that advisor stops recommending adjustments when fitness is good.
  clock_type::set_time(1000);

  RuleBasedAdvisorConfig<float, unsigned long, float> config{
    2.0f,       // overshoot_threshold
    1.0f,       // oscillation_amplitude
    5000UL,     // settling_window
    0.5f,       // settling_tolerance
    0.1f,       // adjustment_factor
    1000UL,     // min_adjustment_interval
    100.0f      // target_fitness = high (easy to achieve, lower is better)
  };

  State<PidOutput<float, float, float>> pid_output_state(make_pid_output(0.0f));

  auto advisor_source = rule_based_advisor<float, float, float, float, unsigned long, float>(
    pid_output_state.get_source_fn(),
    raw_clock_source(),
    config
  );

  TuningAdjustment adjustment = TuningAdjustment::none;
  pull_fn pull = advisor_source([&adjustment](auto v) { adjustment = v; });

  // Initial pull
  pull();

  // Even with oscillation conditions, if fitness is good (low), should cool down
  for (int i = 0; i < 20; i++) {
    clock_type::tick(200);
    // Very small errors lead to low fitness score
    float error = 0.1f * ((i % 2 == 0) ? 1.0f : -1.0f);
    pid_output_state.set(make_pid_output(error), false);
    pull();
  }

  // Should be none because fitness is good (is_cooled_down should be true)
  TEST_ASSERT_EQUAL_MESSAGE(TuningAdjustment::none, adjustment,
    "Should not recommend adjustment when fitness is good (cooled down)");
}

void test_advisor_with_dynamic_fitness_target() {
  // Test that advisor works with dynamic target fitness source.
  clock_type::set_time(1000);

  RuleBasedAdvisorConfig<float, unsigned long, float> config{
    2.0f,       // overshoot_threshold
    1.0f,       // oscillation_amplitude
    5000UL,     // settling_window
    0.5f,       // settling_tolerance
    0.1f,       // adjustment_factor
    1000UL,     // min_adjustment_interval
    0.1f        // target_fitness (unused with dynamic)
  };

  State<PidOutput<float, float, float>> pid_output_state(make_pid_output(0.0f));
  State<float> target_fitness_state(100.0f);  // Start with easy target

  auto advisor_source = rule_based_advisor<float, float, float, float, unsigned long, float>(
    pid_output_state.get_source_fn(),
    raw_clock_source(),
    target_fitness_state.get_source_fn(),
    config
  );

  TuningAdjustment adjustment = TuningAdjustment::none;
  pull_fn pull = advisor_source([&adjustment](auto v) { adjustment = v; });

  // Initial pull
  pull();

  // With high target (easy), even bad performance should cool down
  for (int i = 0; i < 15; i++) {
    clock_type::tick(200);
    float error = (i % 2 == 0) ? 3.0f : -3.0f;
    pid_output_state.set(make_pid_output(error), false);
    pull();
  }

  // Should be none because target fitness is easy to achieve
  TEST_ASSERT_EQUAL_MESSAGE(TuningAdjustment::none, adjustment,
    "Should cool down with easy dynamic target");

  // Now set a stricter target
  target_fitness_state.set(0.01f, false);

  // Continue with oscillations
  for (int i = 0; i < 15; i++) {
    clock_type::tick(200);
    float error = (i % 2 == 0) ? 3.0f : -3.0f;
    pid_output_state.set(make_pid_output(error), false);
    pull();
  }

  // Should now recommend adjustment since target is strict
  TEST_ASSERT_NOT_EQUAL_MESSAGE(TuningAdjustment::none, adjustment,
    "Should recommend adjustment with strict dynamic target");
}

void test_advisor_priority_oscillation_with_overshoot() {
  // Test that oscillation with overshoot triggers decrease_kp (highest priority).
  clock_type::set_time(1000);

  RuleBasedAdvisorConfig<float, unsigned long, float> config{
    1.0f,       // overshoot_threshold (low)
    1.0f,       // oscillation_amplitude (low)
    5000UL,     // settling_window
    0.5f,       // settling_tolerance
    0.1f,       // adjustment_factor
    1000UL,     // min_adjustment_interval
    0.01f       // target_fitness (very strict, won't cool down)
  };

  State<PidOutput<float, float, float>> pid_output_state(make_pid_output(0.0f));

  auto advisor_source = rule_based_advisor<float, float, float, float, unsigned long, float>(
    pid_output_state.get_source_fn(),
    raw_clock_source(),
    config
  );

  TuningAdjustment adjustment = TuningAdjustment::none;
  pull_fn pull = advisor_source([&adjustment](auto v) { adjustment = v; });

  // Initial pull
  pull();

  // Create both oscillation AND overshoot conditions
  for (int i = 0; i < 20; i++) {
    clock_type::tick(200);
    // Oscillating with high amplitude (overshoot)
    float error = (i % 2 == 0) ? 5.0f : -5.0f;
    pid_output_state.set(make_pid_output(error), false);
    pull();
  }

  // Should recommend decrease_kp (Rule 1: oscillation + overshoot)
  TEST_ASSERT_EQUAL_MESSAGE(TuningAdjustment::decrease_kp, adjustment,
    "Should recommend decrease_kp for oscillation with overshoot");
}

void test_advisor_compute_fitness() {
  // Test that fitness is computed correctly from state.
  clock_type::set_time(1000);

  // Create a state with known values
  RuleBasedState<float, unsigned long, float> state{
    2.0f,       // peak_error
    -1.0f,      // valley_error
    5.0f,       // accumulated_error
    10,         // sample_count
    4,          // zero_crossing_count
    true,       // last_error_positive
    1000UL,     // first_sample_time
    1000UL,     // last_adjustment_time
    0.0f,       // current_fitness (will be computed)
    false,      // is_cooled_down
    false,      // first_sample
    1000UL,     // settling_start_time
    false       // in_tolerance_band
  };

  float fitness = compute_fitness_from_state(state);

  // Expected:
  // overshoot = abs(2.0) = 2.0
  // undershoot = abs(-1.0) = 1.0
  // steady_state_error = abs(5.0 / 10) = 0.5
  // oscillations = 4
  // fitness = 2.0*0.3 + 1.0*0.1 + 0.5*0.3 + 4*0.3 = 0.6 + 0.1 + 0.15 + 1.2 = 2.05
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 2.05f, fitness,
    "Fitness should be computed correctly");
}

void test_advisor_first_sample_handling() {
  // Test that first sample initializes state correctly.
  clock_type::set_time(1000);

  RuleBasedAdvisorConfig<float, unsigned long, float> config{
    2.0f, 1.0f, 5000UL, 0.5f, 0.1f, 1000UL, 0.1f
  };

  State<PidOutput<float, float, float>> pid_output_state(make_pid_output(5.0f));

  auto advisor_source = rule_based_advisor<float, float, float, float, unsigned long, float>(
    pid_output_state.get_source_fn(),
    raw_clock_source(),
    config
  );

  TuningAdjustment adjustment = TuningAdjustment::none;
  pull_fn pull = advisor_source([&adjustment](auto v) { adjustment = v; });

  // First pull should initialize state but not recommend anything (not enough samples)
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(TuningAdjustment::none, adjustment,
    "First sample should not recommend adjustment");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_advisor_detects_oscillation);
  RUN_TEST(test_advisor_detects_overshoot);
  RUN_TEST(test_advisor_detects_steady_state_error);
  RUN_TEST(test_advisor_respects_min_adjustment_interval);
  RUN_TEST(test_advisor_cools_down_at_target_fitness);
  RUN_TEST(test_advisor_with_dynamic_fitness_target);
  RUN_TEST(test_advisor_priority_oscillation_with_overshoot);
  RUN_TEST(test_advisor_compute_fitness);
  RUN_TEST(test_advisor_first_sample_handling);
  UNITY_END();
}
