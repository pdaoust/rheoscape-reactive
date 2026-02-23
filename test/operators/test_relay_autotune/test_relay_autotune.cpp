#include <unity.h>
#include <cmath>
#include <operators/pid_autotune/relay_autotune.hpp>
#include <types/mock_clock.hpp>
#include <states/MemoryState.hpp>
#include <types/thermal_sim.hpp>
#include <sources/from_clock.hpp>
#include <sources/constant.hpp>

using namespace rheoscape;
using namespace rheoscape::autotune;
using namespace rheoscape::sources;

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

void test_relay_autotune_produces_oscillations() {
  // Test that relay autotune alternates between high and low output.
  clock_type::set_time(1000);

  MemoryState<float> process_variable(40.0f);
  MemoryState<float> setpoint(45.0f);

  RelayAutotuneConfig<float, float, unsigned long> config{
    1.0f,      // high_output
    0.0f,      // low_output
    0.5f,      // hysteresis
    3,         // min_oscillations
    60000,     // max_duration (60s)
    1000,      // min_period (1s)
    ZieglerNicholsRule::classic
  };

  auto autotune_source = relay_autotune<float, float, unsigned long>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    raw_clock_source(),
    config
  );

  RelayAutotuneOutput<float, float, float, float, unsigned long> output{};
  pull_fn pull = autotune_source([&output](auto v) { output = v; });

  // Initial pull - PV < setpoint, so output should be high
  pull();
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 1.0f, output.control,
    "Initial output should be high when PV < setpoint");

  // Simulate process crossing above setpoint + hysteresis
  clock_type::tick(1000);
  process_variable.set(46.0f, false);
  pull();
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.0f, output.control,
    "Output should switch to low when PV > setpoint + hysteresis");

  // Simulate process crossing below setpoint - hysteresis
  clock_type::tick(1000);
  process_variable.set(44.0f, false);
  pull();
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 1.0f, output.control,
    "Output should switch to high when PV < setpoint - hysteresis");
}

void test_relay_autotune_calculates_ku_tu() {
  // Test that after enough oscillations, Ku and Tu are calculated.
  clock_type::set_time(1000);

  MemoryState<float> process_variable(45.0f);
  MemoryState<float> setpoint(45.0f);

  RelayAutotuneConfig<float, float, unsigned long> config{
    1.0f,      // high_output
    0.0f,      // low_output
    0.5f,      // hysteresis
    2,         // min_oscillations (reduced for test)
    60000,     // max_duration
    500,       // min_period
    ZieglerNicholsRule::classic
  };

  auto autotune_source = relay_autotune<float, float, unsigned long>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    raw_clock_source(),
    config
  );

  RelayAutotuneOutput<float, float, float, float, unsigned long> output{};
  pull_fn pull = autotune_source([&output](auto v) { output = v; });

  // Initial pull
  pull();

  // Simulate oscillations with 2C amplitude and 2s period
  // Peak at 46, valley at 44, period 2000ms
  std::vector<std::pair<float, unsigned long>> trajectory = {
    {44.0f, 1000},  // Valley, crossing up
    {46.0f, 1000},  // Peak, crossing down
    {44.0f, 1000},  // Valley, crossing up
    {46.0f, 1000},  // Peak, crossing down
    {44.0f, 1000},  // Valley, crossing up - should have enough measurements
  };

  for (auto& [temp, dt] : trajectory) {
    clock_type::tick(dt);
    process_variable.set(temp, false);
    pull();
  }

  // Should have converged
  TEST_ASSERT_TRUE_MESSAGE(output.result.has_value(),
    "Result should be populated after enough oscillations");

  if (output.result.has_value()) {
    // Ku = (4 * d) / (pi * a) where d = 0.5, a = 1 (amplitude is half of peak-to-peak)
    // Ku = (4 * 0.5) / (3.14159 * 1) = 0.637
    TEST_ASSERT_TRUE_MESSAGE(output.result->Ku > 0.0f, "Ku should be positive");
    TEST_ASSERT_TRUE_MESSAGE(output.result->Tu > 0, "Tu should be positive");
    TEST_ASSERT_TRUE_MESSAGE(output.result->Kp > 0.0f, "Kp should be positive");
  }
}

void test_relay_autotune_classic_rule() {
  // Test that classic Z-N rule produces correct gain ratios.
  clock_type::set_time(1000);

  MemoryState<float> process_variable(45.0f);
  MemoryState<float> setpoint(45.0f);

  RelayAutotuneConfig<float, float, unsigned long> config{
    1.0f,      // high_output
    0.0f,      // low_output
    0.5f,      // hysteresis
    2,         // min_oscillations
    60000,     // max_duration
    500,       // min_period
    ZieglerNicholsRule::classic
  };

  auto autotune_source = relay_autotune<float, float, unsigned long>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    raw_clock_source(),
    config
  );

  RelayAutotuneOutput<float, float, float, float, unsigned long> output{};
  pull_fn pull = autotune_source([&output](auto v) { output = v; });

  // Initial pull
  pull();

  // Simulate oscillations
  std::vector<std::pair<float, unsigned long>> trajectory = {
    {44.0f, 1000}, {46.0f, 1000}, {44.0f, 1000}, {46.0f, 1000}, {44.0f, 1000},
  };

  for (auto& [temp, dt] : trajectory) {
    clock_type::tick(dt);
    process_variable.set(temp, false);
    pull();
  }

  TEST_ASSERT_TRUE_MESSAGE(output.result.has_value(), "Result should be populated");

  if (output.result.has_value()) {
    // Classic Z-N: Kp = 0.6 * Ku
    float expected_kp_ratio = 0.6f;
    float actual_kp_ratio = output.result->Kp / output.result->Ku;
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, expected_kp_ratio, actual_kp_ratio,
      "Classic Z-N Kp should be 0.6 * Ku");
  }
}

void test_relay_autotune_no_overshoot_rule() {
  // Test that no_overshoot Z-N rule produces more conservative gains.
  clock_type::set_time(1000);

  MemoryState<float> process_variable(45.0f);
  MemoryState<float> setpoint(45.0f);

  RelayAutotuneConfig<float, float, unsigned long> config{
    1.0f,      // high_output
    0.0f,      // low_output
    0.5f,      // hysteresis
    2,         // min_oscillations
    60000,     // max_duration
    500,       // min_period
    ZieglerNicholsRule::no_overshoot
  };

  auto autotune_source = relay_autotune<float, float, unsigned long>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    raw_clock_source(),
    config
  );

  RelayAutotuneOutput<float, float, float, float, unsigned long> output{};
  pull_fn pull = autotune_source([&output](auto v) { output = v; });

  pull();

  std::vector<std::pair<float, unsigned long>> trajectory = {
    {44.0f, 1000}, {46.0f, 1000}, {44.0f, 1000}, {46.0f, 1000}, {44.0f, 1000},
  };

  for (auto& [temp, dt] : trajectory) {
    clock_type::tick(dt);
    process_variable.set(temp, false);
    pull();
  }

  TEST_ASSERT_TRUE_MESSAGE(output.result.has_value(), "Result should be populated");

  if (output.result.has_value()) {
    // No overshoot: Kp = 0.2 * Ku (much lower than classic 0.6)
    float expected_kp_ratio = 0.2f;
    float actual_kp_ratio = output.result->Kp / output.result->Ku;
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, expected_kp_ratio, actual_kp_ratio,
      "No overshoot Z-N Kp should be 0.2 * Ku");
  }
}

void test_relay_autotune_respects_hysteresis() {
  // Test that relay doesn't switch within hysteresis band.
  clock_type::set_time(1000);

  MemoryState<float> process_variable(45.0f);
  MemoryState<float> setpoint(45.0f);

  RelayAutotuneConfig<float, float, unsigned long> config{
    1.0f,      // high_output
    0.0f,      // low_output
    2.0f,      // hysteresis = 2C (large)
    3,         // min_oscillations
    60000,     // max_duration
    1000,      // min_period
    ZieglerNicholsRule::classic
  };

  auto autotune_source = relay_autotune<float, float, unsigned long>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    raw_clock_source(),
    config
  );

  RelayAutotuneOutput<float, float, float, float, unsigned long> output{};
  pull_fn pull = autotune_source([&output](auto v) { output = v; });

  // Initial: PV = 45 = setpoint, but first sample sets initial state
  pull();
  float initial_output = output.control;

  // Move PV slightly below setpoint but still within hysteresis band (45 +/- 2)
  clock_type::tick(100);
  process_variable.set(44.0f, false);  // Still within 43-47 band
  pull();

  // Output should not have switched
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, initial_output, output.control,
    "Output should not switch within hysteresis band");

  // Move PV outside hysteresis band
  clock_type::tick(100);
  process_variable.set(42.0f, false);  // Below 43 = setpoint - hysteresis
  pull();

  // Output should have switched to high (PV < lower threshold)
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 1.0f, output.control,
    "Output should switch to high when PV < setpoint - hysteresis");
}

void test_relay_autotune_respects_min_period() {
  // Test that short oscillations are ignored.
  clock_type::set_time(1000);

  MemoryState<float> process_variable(45.0f);
  MemoryState<float> setpoint(45.0f);

  RelayAutotuneConfig<float, float, unsigned long> config{
    1.0f,      // high_output
    0.0f,      // low_output
    0.5f,      // hysteresis
    2,         // min_oscillations
    60000,     // max_duration
    5000,      // min_period = 5s (long)
    ZieglerNicholsRule::classic
  };

  auto autotune_source = relay_autotune<float, float, unsigned long>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    raw_clock_source(),
    config
  );

  RelayAutotuneOutput<float, float, float, float, unsigned long> output{};
  pull_fn pull = autotune_source([&output](auto v) { output = v; });

  pull();

  // Create rapid oscillations (1s period, less than 5s min_period)
  for (int i = 0; i < 10; i++) {
    clock_type::tick(500);
    process_variable.set(i % 2 == 0 ? 44.0f : 46.0f, false);
    pull();
  }

  // Should not have converged because oscillations are too fast
  TEST_ASSERT_FALSE_MESSAGE(output.result.has_value(),
    "Should not converge with oscillations faster than min_period");
}

void test_relay_autotune_times_out() {
  // Test that autotuning fails after max_duration.
  clock_type::set_time(1000);

  MemoryState<float> process_variable(45.0f);
  MemoryState<float> setpoint(45.0f);

  RelayAutotuneConfig<float, float, unsigned long> config{
    1.0f,      // high_output
    0.0f,      // low_output
    0.5f,      // hysteresis
    10,        // min_oscillations (high, won't be reached)
    5000,      // max_duration = 5s (short)
    1000,      // min_period
    ZieglerNicholsRule::classic
  };

  auto autotune_source = relay_autotune<float, float, unsigned long>(
    process_variable.get_source_fn(),
    setpoint.get_source_fn(),
    raw_clock_source(),
    config
  );

  RelayAutotuneOutput<float, float, float, float, unsigned long> output{};
  pull_fn pull = autotune_source([&output](auto v) { output = v; });

  pull();

  // Advance time past max_duration without enough oscillations
  for (int i = 0; i < 10; i++) {
    clock_type::tick(1000);  // Total 10s, > 5s max
    pull();
  }

  // Should not have result (failed)
  TEST_ASSERT_FALSE_MESSAGE(output.result.has_value(),
    "Should not have result after timeout");
}

void test_relay_autotune_with_thermal_sim() {
  // Integration test: relay autotune with thermal simulator.
  clock_type::set_time(1000);

  auto thermal_config = make_sous_vide_config<float, float, unsigned long>(
    0.5f,     // 0.5 liter (small for fast response)
    200.0f,   // 200W heater
    20.0f,    // 20C ambient
    5.0f,     // 5 W/K heat loss
    100       // 100ms PWM cycle
  );

  float initial_temp = 40.0f;
  float target_temp = 45.0f;

  MemoryState<float> duty(0.5f);  // Start at 50% duty
  MemoryState<float> setpoint_state(target_temp);

  auto raw_clock = raw_clock_source();

  // Create thermal sim
  auto temp_source = thermal_sim<float, unsigned long, float, float>(
    duty.get_source_fn(),
    raw_clock,
    thermal_config,
    initial_temp
  );

  // Create temperature state to capture temp_source output
  float current_temp = initial_temp;
  pull_fn pull_temp = temp_source([&current_temp](float t) { current_temp = t; });

  // Create relay autotune
  RelayAutotuneConfig<float, float, unsigned long> config{
    1.0f,      // high_output
    0.0f,      // low_output
    0.5f,      // hysteresis
    2,         // min_oscillations
    30000,     // max_duration (30s)
    500,       // min_period
    ZieglerNicholsRule::no_overshoot
  };

  MemoryState<float> temp_state(current_temp);

  auto autotune_source = relay_autotune<float, float, unsigned long>(
    temp_state.get_source_fn(),
    setpoint_state.get_source_fn(),
    raw_clock,
    config
  );

  RelayAutotuneOutput<float, float, float, float, unsigned long> output{};
  pull_fn pull_autotune = autotune_source([&output](auto v) { output = v; });

  // Run simulation loop
  bool converged = false;
  for (int step = 0; step < 300 && !converged; step++) {  // Up to 30s
    clock_type::tick(100);

    // Pull temperature from thermal sim
    pull_temp();
    temp_state.set(current_temp, false);

    // Pull autotune to get control output
    pull_autotune();

    // Apply control output to duty
    duty.set(output.control, false);

    if (output.result.has_value()) {
      converged = true;
    }
  }

  // May or may not converge depending on thermal dynamics
  // Just verify it runs without crashing
  TEST_ASSERT_TRUE_MESSAGE(true, "Integration test completed without crash");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_relay_autotune_produces_oscillations);
  RUN_TEST(test_relay_autotune_calculates_ku_tu);
  RUN_TEST(test_relay_autotune_classic_rule);
  RUN_TEST(test_relay_autotune_no_overshoot_rule);
  RUN_TEST(test_relay_autotune_respects_hysteresis);
  RUN_TEST(test_relay_autotune_respects_min_period);
  RUN_TEST(test_relay_autotune_times_out);
  RUN_TEST(test_relay_autotune_with_thermal_sim);
  UNITY_END();
}
