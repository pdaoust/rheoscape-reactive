#include <unity.h>
#include <cmath>
#include <operators/pid.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <types/thermal_sim.hpp>
#include <sources/from_clock.hpp>
#include <sources/constant.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

// Use time_point type for proper chrono integration
using clock_type = mock_clock_ulong_millis;
using time_point = clock_type::time_point;
using duration_type = clock_type::duration;

// Helper to convert chrono duration to float seconds
auto duration_to_seconds = [](duration_type d) {
  return std::chrono::duration<float>(d).count();
};

void test_pid_basic_proportional_control() {
  // Test that with only P gain, error produces proportional output
  clock_type::set_time(1000);  // Start at 1000ms to avoid zero timestamp issues
  auto clock = from_clock<clock_type>();

  State<float> process_variable(20.0f);
  State<float> setpoint(25.0f);
  State<PidWeights<float, float, float>> weights(PidWeights<float, float, float>{ 1.0f, 0.0f, 0.0f });

  auto pid_source = pid<float, time_point>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    weights.get_source_fn(),
    std::nullopt,
    duration_to_seconds
  );

  float last_control = 0.0f;
  pull_fn pull = pid_source([&last_control](float v) { last_control = v; });

  // First pull establishes initial state
  pull();
  clock_type::tick(100);

  // Second pull calculates actual PID
  pull();

  // Error = 25 - 20 = 5, Kp = 1.0, so control should be ~5.0
  TEST_ASSERT_TRUE_MESSAGE(last_control > 0.0f, "Control should be positive when process < setpoint with positive Kp");
}

void test_pid_integral_accumulates() {
  clock_type::set_time(1000);
  auto clock = from_clock<clock_type>();

  State<float> process_variable(20.0f);
  State<float> setpoint(25.0f);
  // Only integral gain (Ki in units per second, so use a larger value since dt is in seconds)
  State<PidWeights<float, float, float>> weights(PidWeights<float, float, float>{ 0.0f, 10.0f, 0.0f });

  auto pid_source = pid<float, time_point>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    weights.get_source_fn(),
    std::nullopt,
    duration_to_seconds
  );

  float last_control = 0.0f;
  pull_fn pull = pid_source([&last_control](float v) { last_control = v; });

  // Establish initial state
  pull();
  clock_type::tick(100);
  pull();
  float control_after_100ms = last_control;

  // Continue with same error
  clock_type::tick(100);
  pull();
  float control_after_200ms = last_control;

  // Integral should have accumulated more
  TEST_ASSERT_TRUE_MESSAGE(
    control_after_200ms > control_after_100ms,
    "Integral control should increase over time with constant error"
  );
}

void test_pid_clamps_output() {
  clock_type::set_time(1000);
  auto clock = from_clock<clock_type>();

  State<float> process_variable(0.0f);
  State<float> setpoint(100.0f);  // Large error
  State<PidWeights<float, float, float>> weights(PidWeights<float, float, float>{ 10.0f, 0.0f, 0.0f });

  // Clamp output between 0 and 1
  Range<float> clamp_range{ 0.0f, 1.0f };

  auto pid_source = pid<float, time_point>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    weights.get_source_fn(),
    clamp_range,
    duration_to_seconds
  );

  float last_control = 0.0f;
  pull_fn pull = pid_source([&last_control](float v) { last_control = v; });

  pull();
  clock_type::tick(100);
  pull();

  // With Kp=10 and error=100, unclamped output would be 1000
  // But it should be clamped to 1.0
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 1.0f, last_control, "Output should be clamped to max");
}

void test_pid_clamps_output_negative() {
  clock_type::set_time(1000);
  auto clock = from_clock<clock_type>();

  State<float> process_variable(100.0f);  // Process above setpoint
  State<float> setpoint(0.0f);
  State<PidWeights<float, float, float>> weights(PidWeights<float, float, float>{ 10.0f, 0.0f, 0.0f });

  // Clamp output between 0 and 1
  Range<float> clamp_range{ 0.0f, 1.0f };

  auto pid_source = pid<float, time_point>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    weights.get_source_fn(),
    clamp_range,
    duration_to_seconds
  );

  float last_control = 0.5f;
  pull_fn pull = pid_source([&last_control](float v) { last_control = v; });

  pull();
  clock_type::tick(100);
  pull();

  // Error is negative, so control would be -1000 unclamped
  // Should be clamped to 0.0
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 0.0f, last_control, "Output should be clamped to min");
}

void test_pid_detailed_exposes_all_terms() {
  clock_type::set_time(1000);
  auto clock = from_clock<clock_type>();

  State<float> process_variable(20.0f);
  State<float> setpoint(25.0f);
  State<PidWeights<float, float, float>> weights(PidWeights<float, float, float>{ 1.0f, 10.0f, 0.1f });

  auto pid_source = pid_detailed<float, time_point>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    weights.get_source_fn(),
    std::nullopt,
    duration_to_seconds
  );

  PidOutput<float, float, float> last_output{};
  pull_fn pull = pid_source([&last_output](auto v) { last_output = v; });

  pull();
  clock_type::tick(100);
  pull();

  // Check that all fields are accessible and sensible
  TEST_ASSERT_TRUE_MESSAGE(last_output.error > 0.0f, "Error should be positive (setpoint > pv)");
  TEST_ASSERT_TRUE_MESSAGE(last_output.p_term > 0.0f, "P term should be positive");
  // Control should be sum of p, i, d terms
  float expected_control = last_output.p_term + last_output.i_term + last_output.d_term;
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, expected_control, last_output.control, "Control should be sum of P+I+D terms");
  TEST_ASSERT_FALSE_MESSAGE(last_output.is_saturated, "Should not be saturated without clamping");
}

void test_pid_detailed_shows_saturation() {
  clock_type::set_time(1000);
  auto clock = from_clock<clock_type>();

  State<float> process_variable(0.0f);
  State<float> setpoint(100.0f);  // Large error
  State<PidWeights<float, float, float>> weights(PidWeights<float, float, float>{ 10.0f, 0.0f, 0.0f });

  Range<float> clamp_range{ 0.0f, 1.0f };

  auto pid_source = pid_detailed<float, time_point>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    weights.get_source_fn(),
    clamp_range,
    duration_to_seconds
  );

  PidOutput<float, float, float> last_output{};
  pull_fn pull = pid_source([&last_output](auto v) { last_output = v; });

  pull();
  clock_type::tick(100);
  pull();

  TEST_ASSERT_TRUE_MESSAGE(last_output.is_saturated, "Should be saturated when control is clamped");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 1.0f, last_output.control, "Control should be clamped to max");
}

void test_pid_dynamic_weight_change() {
  clock_type::set_time(1000);
  auto clock = from_clock<clock_type>();

  State<float> process_variable(20.0f);
  State<float> setpoint(25.0f);
  State<PidWeights<float, float, float>> weights(PidWeights<float, float, float>{ 1.0f, 0.0f, 0.0f });

  auto pid_source = pid<float, time_point>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    weights.get_source_fn(),
    std::nullopt,
    duration_to_seconds
  );

  float last_control = 0.0f;
  pull_fn pull = pid_source([&last_control](float v) { last_control = v; });

  // Initial pull
  pull();
  clock_type::tick(100);
  pull();
  float control_with_kp_1 = last_control;

  // Double the Kp
  // Use set(value, false) to avoid implicit push - we want pull-based behavior
  weights.set(PidWeights<float, float, float>{ 2.0f, 0.0f, 0.0f }, false);
  clock_type::tick(100);
  pull();
  float control_with_kp_2 = last_control;

  // With doubled Kp and same error, output should be approximately doubled
  TEST_ASSERT_TRUE_MESSAGE(
    control_with_kp_2 > control_with_kp_1,
    "Control should increase when Kp is increased"
  );
}

void test_pid_responds_to_setpoint_change() {
  clock_type::set_time(1000);
  auto clock = from_clock<clock_type>();

  State<float> process_variable(50.0f);
  State<float> setpoint(50.0f);  // Initially at setpoint
  State<PidWeights<float, float, float>> weights(PidWeights<float, float, float>{ 1.0f, 0.0f, 0.0f });

  auto pid_source = pid<float, time_point>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    weights.get_source_fn(),
    std::nullopt,
    duration_to_seconds
  );

  float last_control = 0.0f;
  pull_fn pull = pid_source([&last_control](float v) { last_control = v; });

  // At setpoint, error should be ~0
  pull();
  clock_type::tick(100);
  pull();
  float control_at_setpoint = last_control;

  // Change setpoint upward
  // Use set(value, false) to avoid implicit push - we want pull-based behavior
  setpoint.set(60.0f, false);
  clock_type::tick(100);
  pull();
  float control_after_setpoint_change = last_control;

  // Control should increase because error increased
  TEST_ASSERT_TRUE_MESSAGE(
    control_after_setpoint_change > control_at_setpoint,
    "Control should increase when setpoint increases above process variable"
  );
}

void test_pid_derivative_opposes_error_change() {
  // This test verifies that derivative action produces negative d_term
  // when error is decreasing (i.e., approaching setpoint)
  clock_type::set_time(1000);
  auto clock = from_clock<clock_type>();

  State<float> process_variable(20.0f);
  State<float> setpoint(25.0f);
  // Only derivative, no P or I
  State<PidWeights<float, float, float>> weights(PidWeights<float, float, float>{ 0.0f, 0.0f, 1.0f });

  auto pid_source = pid_detailed<float, time_point>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    weights.get_source_fn(),
    std::nullopt,
    duration_to_seconds
  );

  PidOutput<float, float, float> output{};
  pull_fn pull = pid_source([&output](auto v) { output = v; });

  // First pull establishes initial state (error = 5)
  pull();
  float error_1 = output.error;
  float d_term_1 = output.d_term;

  // Error should be 5 on first pull
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1f, 5.0f, error_1, "Error should be 5 on first pull");

  // D term on first pull: derivative = (5 - 0) / 1.0 = 5, d_term = 1.0 * 5 = 5
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(1.0f, 5.0f, d_term_1, "D term should be ~5 on first pull (error rising from 0 to 5 over 1s)");

  clock_type::tick(100);

  // Second pull with same error
  pull();
  float error_2 = output.error;
  float d_term_2 = output.d_term;

  // Error should still be 5
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1f, 5.0f, error_2, "Error should still be 5 on second pull");

  // D term should be ~0 when error is steady
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1f, 0.0f, d_term_2, "D term should be ~0 when error is steady");

  // Now error decreases (approaching setpoint)
  // Use set(value, false) to avoid implicit push - we want pull-based behavior
  process_variable.set(23.0f, false);  // error goes from 5 to 2
  clock_type::tick(100);
  pull();
  float error_3 = output.error;
  float d_term_3 = output.d_term;

  // Error should be 2 (setpoint 25 - pv 23)
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1f, 2.0f, error_3, "Error should be 2 after pv change");

  // Expected: derivative = (2 - 5) / 0.1 = -30, so d_term = 1.0 * -30 = -30
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(5.0f, -30.0f, d_term_3, "D term should be ~-30 when error decreases from 5 to 2 over 0.1s");
}

// =============================================================================
// Thermal Simulator Integration Tests
// =============================================================================

// Helper source that returns raw milliseconds from mock_clock.
// This is needed because thermal_sim expects raw time, not time_point.
source_fn<unsigned long> raw_clock_source() {
  return [](push_fn<unsigned long> push) -> pull_fn {
    return [push]() {
      push(clock_type::now().time_since_epoch().count());
    };
  };
}

// Helper to run a closed-loop PID + thermal sim simulation.
// Returns final temperature after specified number of steps.
float run_thermal_sim_with_pid(
  ThermalSimConfig<float, float, unsigned long> thermal_config,
  float initial_temp,
  float target_temp,
  PidWeights<float, float, float> weights,
  int num_steps,
  unsigned long step_ms,
  float* disturbance_power = nullptr,
  int disturbance_start_step = -1,
  int disturbance_end_step = -1
) {
  clock_type::set_time(1000);  // Start at 1s

  // State variables for closed loop
  State<float> duty(0.0f);
  State<float> temperature(initial_temp);
  State<float> setpoint(target_temp);
  State<PidWeights<float, float, float>> pid_weights(weights);
  State<float> disturbance(0.0f);

  // Create clock sources
  // Thermal sim uses raw milliseconds
  auto raw_clock = raw_clock_source();
  // PID uses time_point for duration conversion
  auto clock = from_clock<clock_type>();

  // Create thermal sim with disturbance
  auto temp_source = thermal_sim<float, unsigned long, float, float>(
    duty.get_source_fn(),
    raw_clock,
    disturbance.get_source_fn(),
    thermal_config,
    initial_temp
  );

  // Create PID controller
  auto pid_source = pid<float, time_point>(
    temperature.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    pid_weights.get_source_fn(),
    Range<float>{ 0.0f, 1.0f },  // Duty cycle 0-1
    duration_to_seconds
  );

  // Bind and create pull functions
  float current_temp = initial_temp;
  float current_duty = 0.0f;

  pull_fn pull_temp = temp_source([&current_temp](float t) { current_temp = t; });
  pull_fn pull_pid = pid_source([&current_duty](float d) { current_duty = d; });

  // Initial pull to establish state
  pull_temp();
  temperature.set(current_temp, false);
  pull_pid();
  duty.set(current_duty, false);

  // Simulation loop
  for (int step = 0; step < num_steps; step++) {
    clock_type::tick(step_ms);

    // Handle disturbance
    if (disturbance_power != nullptr &&
        step >= disturbance_start_step && step < disturbance_end_step) {
      disturbance.set(*disturbance_power, false);
    } else {
      disturbance.set(0.0f, false);
    }

    // Pull new temperature from thermal sim
    pull_temp();
    temperature.set(current_temp, false);

    // Pull new duty from PID
    pull_pid();
    duty.set(current_duty, false);
  }

  return current_temp;
}

void test_pid_thermal_sim_reaches_setpoint() {
  // Test that PID + thermal sim reaches setpoint.
  // Use a small thermal system for faster testing.
  //
  // Parameters: 1L water, 500W heater, short time constant
  auto thermal_config = make_sous_vide_config<float, float, unsigned long>(
    1.0f,     // 1 liter (4186 J/K)
    500.0f,   // 500W heater
    20.0f,    // 20C ambient
    10.0f,    // 10 W/K heat loss (fast)
    100       // 100ms PWM cycle
  );

  float initial_temp = 20.0f;
  float target_temp = 40.0f;

  // PID weights tuned for this system
  // Time constant: tau = C/k = 4186/10 = 418.6s
  // Use aggressive gains for faster testing
  PidWeights<float, float, float> weights{
    0.1f,     // Kp: 10% duty per degree error
    0.001f,   // Ki: slow integral
    5.0f      // Kd: anticipate changes
  };

  // Run for enough steps to reach setpoint (simulate ~10 minutes in 100ms steps)
  int num_steps = 6000;  // 600 seconds = 10 minutes
  unsigned long step_ms = 100;

  float final_temp = run_thermal_sim_with_pid(
    thermal_config, initial_temp, target_temp, weights,
    num_steps, step_ms
  );

  // Should reach within 2C of setpoint
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
    2.0f, target_temp, final_temp,
    "Temperature should reach within 2C of setpoint"
  );
}

void test_pid_thermal_sim_rejects_disturbance() {
  // Test that PID recovers after a disturbance.
  auto thermal_config = make_sous_vide_config<float, float, unsigned long>(
    1.0f,     // 1 liter
    500.0f,   // 500W heater
    20.0f,    // 20C ambient
    10.0f,    // 10 W/K heat loss
    100       // 100ms PWM cycle
  );

  float initial_temp = 35.0f;  // Start closer to setpoint
  float target_temp = 40.0f;

  PidWeights<float, float, float> weights{
    0.1f,     // Kp
    0.001f,   // Ki
    5.0f      // Kd
  };

  // First reach steady state (3 minutes)
  int warmup_steps = 1800;

  // Then apply disturbance (1 minute) then recover (3 minutes)
  int total_steps = 6000;
  float disturbance = -200.0f;  // -200W cooling disturbance
  int dist_start = warmup_steps;
  int dist_end = warmup_steps + 600;  // 1 minute disturbance

  unsigned long step_ms = 100;

  float final_temp = run_thermal_sim_with_pid(
    thermal_config, initial_temp, target_temp, weights,
    total_steps, step_ms,
    &disturbance, dist_start, dist_end
  );

  // Should recover to within 3C of setpoint after disturbance
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
    3.0f, target_temp, final_temp,
    "Temperature should recover to within 3C of setpoint after disturbance"
  );
}

void test_pid_thermal_sim_tracks_setpoint_change() {
  // Test that PID tracks a setpoint change.
  auto thermal_config = make_sous_vide_config<float, float, unsigned long>(
    1.0f,     // 1 liter
    500.0f,   // 500W heater
    20.0f,    // 20C ambient
    10.0f,    // 10 W/K heat loss
    100       // 100ms PWM cycle
  );

  float initial_temp = 35.0f;
  float first_setpoint = 40.0f;
  float second_setpoint = 45.0f;

  PidWeights<float, float, float> weights{
    0.1f,     // Kp
    0.001f,   // Ki
    5.0f      // Kd
  };

  clock_type::set_time(1000);

  State<float> duty(0.0f);
  State<float> temperature(initial_temp);
  State<float> setpoint(first_setpoint);
  State<PidWeights<float, float, float>> pid_weights(weights);

  // Use raw clock for thermal sim, time_point clock for PID
  auto raw_clock = raw_clock_source();
  auto clock = from_clock<clock_type>();

  auto temp_source = thermal_sim<float, unsigned long, float, float>(
    duty.get_source_fn(),
    raw_clock,
    thermal_config,
    initial_temp
  );

  auto pid_source = pid<float, time_point>(
    temperature.get_source_fn(),
    setpoint.get_source_fn(),
    clock,
    pid_weights.get_source_fn(),
    Range<float>{ 0.0f, 1.0f },
    duration_to_seconds
  );

  float current_temp = initial_temp;
  float current_duty = 0.0f;

  pull_fn pull_temp = temp_source([&current_temp](float t) { current_temp = t; });
  pull_fn pull_pid = pid_source([&current_duty](float d) { current_duty = d; });

  // Initial pull
  pull_temp();
  temperature.set(current_temp, false);
  pull_pid();
  duty.set(current_duty, false);

  unsigned long step_ms = 100;

  // Reach first setpoint (3 minutes)
  for (int step = 0; step < 1800; step++) {
    clock_type::tick(step_ms);
    pull_temp();
    temperature.set(current_temp, false);
    pull_pid();
    duty.set(current_duty, false);
  }

  // Change setpoint
  setpoint.set(second_setpoint, false);

  // Track new setpoint (6 minutes for slower systems)
  for (int step = 0; step < 3600; step++) {
    clock_type::tick(step_ms);
    pull_temp();
    temperature.set(current_temp, false);
    pull_pid();
    duty.set(current_duty, false);
  }

  // Should track the new setpoint.
  // Allow 4C tolerance since thermal systems take time to reach equilibrium.
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
    4.0f, second_setpoint, current_temp,
    "Temperature should track new setpoint within 4C"
  );
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_pid_basic_proportional_control);
  RUN_TEST(test_pid_integral_accumulates);
  RUN_TEST(test_pid_clamps_output);
  RUN_TEST(test_pid_clamps_output_negative);
  RUN_TEST(test_pid_detailed_exposes_all_terms);
  RUN_TEST(test_pid_detailed_shows_saturation);
  RUN_TEST(test_pid_dynamic_weight_change);
  RUN_TEST(test_pid_responds_to_setpoint_change);
  RUN_TEST(test_pid_derivative_opposes_error_change);
  // Thermal simulator integration tests
  RUN_TEST(test_pid_thermal_sim_reaches_setpoint);
  RUN_TEST(test_pid_thermal_sim_rejects_disturbance);
  RUN_TEST(test_pid_thermal_sim_tracks_setpoint_change);
  UNITY_END();
}
