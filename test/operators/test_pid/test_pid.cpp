#include <unity.h>
#include <cmath>
#include <operators/pid.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <sources/from_clock.hpp>
#include <sources/constant.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

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
  UNITY_END();
}
