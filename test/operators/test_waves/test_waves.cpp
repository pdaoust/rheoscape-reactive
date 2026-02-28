#include <unity.h>
#include <functional>
#include <operators/waves.hpp>
#include <sources/constant.hpp>
#include <sources/sequence.hpp>
#include <sources/from_clock.hpp>
#include <states/MemoryState.hpp>
#include <types/mock_clock.hpp>
#include <fmt/format.h>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;
using namespace rheoscape::states;

void test_wave_waves() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto phase_shift_source = constant(0);
  auto wave_source = wave(input_source, period_source, [](float value) { return value; }, phase_shift_source);

  float pushed_value = 0;
  auto pull = wave_source([&pushed_value](float v) { pushed_value = v; });

  for (int i = 0; i < 200; i ++) {
    pull();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE((float)(i % 100) / 100, pushed_value, "Should calculate normalised phase");
    if (i % 100 == 0) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pushed_value, "Should pass zero phase smoke test");
    } else if (i % 100 == 25) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.25f, pushed_value, "Should pass quarter phase smoke test");
    } else if (i % 100 == 50) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, pushed_value, "Should pass half phase smoke test");
    } else if (i % 100 == 75) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.75f, pushed_value, "Should pass three quarters phase smoke test");
    }
  }
}

void test_wave_shifts_phase() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto phase_shift_source = constant(75);
  auto wave_source = wave(input_source, period_source, [](float value) { return value; }, phase_shift_source);

  float pushed_value = 0;
  auto pull = wave_source([&pushed_value](float v) { pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    if (i < 75) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE((float)i / 100 + 0.25f, pushed_value, "Should calculate normalised phase value");
    } else {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE((float)i / 100 - 0.75f, pushed_value, "Should calculate normalised phase value");
    }
  }
}

void test_sine_wave_waves() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto phase_shift_source = constant(0);
  auto sine_source = sine_wave(input_source, period_source, phase_shift_source);

  float pushed_value = 0;
  auto pull = sine_source([&pushed_value](float v) { pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    if (i == 0 || i == 50) {
      // sin(pi) != 0
      TEST_ASSERT_TRUE_MESSAGE(pushed_value < 0.000000001f, "Should pass zero crossing smoke test");
    } else if (i == 25) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, pushed_value, "Should pass trough smoke test");
    } else if (i == 75) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-1.0f, pushed_value, "Should pass peak smoke test");
    } else {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(sin((float)i / 100 * M_PI * 2), pushed_value, "Should calculate sin");
    }
  }
}

void test_square_wave_waves() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto phase_shift_source = constant(0);
  auto square_source = square_wave(input_source, period_source, phase_shift_source);

  float pushed_value = 0;
  int pushed_count = 0;
  auto pull = square_source([&pushed_value, &pushed_count](float v) { pushed_count ++; pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushed_count, "Should have pushed the right number of times");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(i < 50 ? 1.0f : -1.0f, pushed_value, std::format("Should calculate square wave correctly at {}", i).c_str());
  }
}

void test_sawtooth_wave_waves() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto phase_shift_source = constant(0);
  auto sawtooth_source = sawtooth_wave(input_source, period_source, phase_shift_source);

  float pushed_value = 0;
  auto pull = sawtooth_source([&pushed_value](float v) { pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    if (i == 0) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-1.0f, pushed_value, "Should pass start smoke test");
    } else if (i == 50) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pushed_value, "Should pass zero crossing smoke test");
    } else if (i == 99) {
      TEST_ASSERT_TRUE_MESSAGE(pushed_value >= 0.98f, "Should pass end smoke test");
    }
  }
}

void test_triangle_wave_waves() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto phase_shift_source = constant(0);
  auto triangle_source = triangle_wave(input_source, period_source, phase_shift_source);

  float pushed_value = 0;
  auto pull = triangle_source([&pushed_value](float v) { pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    if (i == 0 || i == 50) {
      // sin(pi) != 0
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pushed_value, "Should pass zero crossing smoke test");
    } else if (i == 25) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, pushed_value, "Should pass peak smoke test");
    } else if (i == 75) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-1.0f, pushed_value, "Should pass trough smoke test");
    }
  }
}

void test_pwm_wave_waves() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto phase_shift_source = constant(0);
  MemoryState<float> duty_state(0.0f, false);
  auto pwm_source = pwm_wave(input_source, period_source, duty_state.get_source_fn(false), phase_shift_source);

  bool pushed_value;
  auto pull = pwm_source([&pushed_value](auto v) { pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_FALSE_MESSAGE(pushed_value, fmt::format("Should calculate PWM wave with 0 duty at {}", i).c_str());
  }

  duty_state.set(0.25f, false);
  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i < 25, pushed_value, fmt::format("Should calculate PWM wave with 0.25 duty at {}", i).c_str());
  }

  duty_state.set(0.75f, false);
  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i < 75, pushed_value, fmt::format("Should calculate PWM wave with 0.75 duty at {}", i).c_str());
  }

  duty_state.set(1.0f, false);
  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0, pushed_value, "Should calculate PWM wave with 1.0 duty");
  }
}

// No-phase-shift overload tests.

void test_wave_no_phase() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto wave_source = wave(input_source, period_source, [](float value) { return value; });

  float pushed_value = 0;
  auto pull = wave_source([&pushed_value](float v) { pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE((float)(i % 100) / 100, pushed_value, "Should calculate normalised phase");
  }
}

void test_sine_wave_no_phase() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto sine_source = sine_wave(input_source, period_source);

  float pushed_value = 0;
  auto pull = sine_source([&pushed_value](float v) { pushed_value = v; });

  pull();
  TEST_ASSERT_TRUE_MESSAGE(pushed_value < 0.000000001f, "Should start at zero");
  for (int i = 1; i < 25; i++) pull();
  pull();
  // i == 25
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, pushed_value, "Should peak at 1");
}

void test_sawtooth_wave_no_phase() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto sawtooth_source = sawtooth_wave(input_source, period_source);

  float pushed_value = 0;
  auto pull = sawtooth_source([&pushed_value](float v) { pushed_value = v; });

  pull();
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-1.0f, pushed_value, "Should start at -1");
}

void test_triangle_wave_no_phase() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto triangle_source = triangle_wave(input_source, period_source);

  float pushed_value = 0;
  auto pull = triangle_source([&pushed_value](float v) { pushed_value = v; });

  pull();
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pushed_value, "Should start at zero");
}

void test_square_wave_no_phase() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto square_source = square_wave(input_source, period_source);

  float pushed_value = 0;
  auto pull = square_source([&pushed_value](float v) { pushed_value = v; });

  pull();
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, pushed_value, "Should start high");
}

void test_pwm_wave_no_phase() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto pwm_source = pwm_wave(input_source, period_source, constant(0.5f));

  bool pushed_value;
  auto pull = pwm_source([&pushed_value](auto v) { pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i < 50, pushed_value, fmt::format("Should calculate PWM wave with 0.5 duty at {}", i).c_str());
  }
}

// Pipe factory tests.

void test_wave_pipe() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto wave_source = input_source | wave(period_source, [](float value) { return value; });

  float pushed_value = 0;
  auto pull = wave_source([&pushed_value](float v) { pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE((float)(i % 100) / 100, pushed_value, "Should calculate normalised phase");
  }
}

void test_sine_wave_pipe() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto sine_source = input_source | sine_wave(period_source);

  float pushed_value = 0;
  auto pull = sine_source([&pushed_value](float v) { pushed_value = v; });

  pull();
  TEST_ASSERT_TRUE_MESSAGE(pushed_value < 0.000000001f, "Should start at zero");
}

void test_sawtooth_wave_pipe() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto sawtooth_source = input_source | sawtooth_wave(period_source);

  float pushed_value = 0;
  auto pull = sawtooth_source([&pushed_value](float v) { pushed_value = v; });

  pull();
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-1.0f, pushed_value, "Should start at -1");
}

void test_triangle_wave_pipe() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto triangle_source = input_source | triangle_wave(period_source);

  float pushed_value = 0;
  auto pull = triangle_source([&pushed_value](float v) { pushed_value = v; });

  pull();
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pushed_value, "Should start at zero");
}

void test_square_wave_pipe() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto square_source = input_source | square_wave(period_source);

  float pushed_value = 0;
  auto pull = square_source([&pushed_value](float v) { pushed_value = v; });

  pull();
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, pushed_value, "Should start high");
}

void test_pwm_wave_pipe() {
  auto input_source = sequence_open(0, 1);
  auto period_source = constant(100);
  auto pwm_source = input_source | pwm_wave(period_source, constant(0.5f));

  bool pushed_value;
  auto pull = pwm_source([&pushed_value](auto v) { pushed_value = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i < 50, pushed_value, fmt::format("Should calculate PWM pipe with 0.5 duty at {}", i).c_str());
  }
}

void test_square_wave_with_chrono_clock() {
  // Smoke test: verify wave operators compile and work
  // when the input source emits chrono time_points
  // and the period/phase sources emit chrono durations.
  using clock = mock_clock_ulong_millis;
  using duration = clock::duration;

  clock::set_time(0);
  auto input_source = from_clock<clock>();
  auto period_source = constant(duration(100));
  auto phase_shift_source = constant(duration(0));
  auto square_source = square_wave(input_source, period_source, phase_shift_source);

  float pushed_value = 0;
  int pushed_count = 0;
  auto pull = square_source([&pushed_value, &pushed_count](float v) { pushed_count++; pushed_value = v; });

  for (int i = 0; i < 100; i++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushed_count, "Should have pushed the right number of times");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(
      i < 50 ? 1.0f : -1.0f, pushed_value,
      fmt::format("Should calculate square wave correctly at tick {}", i).c_str()
    );
    clock::tick();
  }
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_wave_waves);
  RUN_TEST(test_wave_shifts_phase);
  RUN_TEST(test_sine_wave_waves);
  RUN_TEST(test_square_wave_waves);
  RUN_TEST(test_sawtooth_wave_waves);
  RUN_TEST(test_triangle_wave_waves);
  RUN_TEST(test_pwm_wave_waves);
  RUN_TEST(test_wave_no_phase);
  RUN_TEST(test_sine_wave_no_phase);
  RUN_TEST(test_sawtooth_wave_no_phase);
  RUN_TEST(test_triangle_wave_no_phase);
  RUN_TEST(test_square_wave_no_phase);
  RUN_TEST(test_pwm_wave_no_phase);
  RUN_TEST(test_wave_pipe);
  RUN_TEST(test_sine_wave_pipe);
  RUN_TEST(test_sawtooth_wave_pipe);
  RUN_TEST(test_triangle_wave_pipe);
  RUN_TEST(test_square_wave_pipe);
  RUN_TEST(test_pwm_wave_pipe);
  RUN_TEST(test_square_wave_with_chrono_clock);
  UNITY_END();
}