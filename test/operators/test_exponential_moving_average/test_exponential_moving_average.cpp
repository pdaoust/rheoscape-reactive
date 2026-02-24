#include <unity.h>
#include <algorithm>
#include <functional>
#include <operators/exponential_moving_average.hpp>
#include <operators/map.hpp>
#include <operators/waves.hpp>
#include <sources/constant.hpp>
#include <sources/from_clock.hpp>
#include <sources/sequence.hpp>
#include <types/mock_clock.hpp>
#include <states/MemoryState.hpp>
#include <fmt/format.h>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;
using namespace rheoscape::states;

void test_exponential_moving_average_stays_stable() {
  // We're going to make it as simple as possible
  // and use an integer counter for the clock source.
  auto clock_source = sequence_open(1, 1);
  auto value_source = constant(5);
  auto time_constant_source = constant(10);
  auto avg = exponential_moving_average(value_source, clock_source, time_constant_source);

  int value = 0;
  int push_count = 0;
  auto pull = avg([&value, &push_count](int v) {
    value = v;
    push_count ++;
  });

  for (int i = 1; i <= 1000; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, push_count, "Should have pushed");
    TEST_ASSERT_EQUAL_MESSAGE(5, value, "Value should stay stable");
  }
}

void test_exponential_moving_average_accommodates_discontinuous_time_jumps() {
  auto clock_source = from_clock<mock_clock_ulong_millis>()
    | map([](mock_clock_ulong_millis::time_point v) { return v.time_since_epoch().count(); });
  auto period_source = MemoryState(100UL, false);
  auto value_source = sine_wave(clock_source, period_source.get_source_fn(), constant(0UL))
    | map([](float v) { return v * 10; });
  auto time_constant_source = constant(400UL);
  auto avg = exponential_moving_average(value_source, clock_source, time_constant_source);

  float value = 0.0f;
  auto pull = avg([&value](float v) { value = v; });

  for (int i = 1; i <= 10000; i ++) {
    mock_clock_ulong_millis::tick(rand() % 10);
    pull();
    if (i % 100 == 0) {
      // Over the course of one period, it should roughly even out,
      // as long as the average sample coarseness is suitably lower than the period.
      TEST_ASSERT_TRUE_MESSAGE(abs(value) < 2.0f, fmt::format("Value {} should be between -2 and +2", value).c_str());
    }
  }

  period_source.set(800, false);
  float min_value = 0;
  float max_value = 0;
  for (int i = 1; i <= 10000; i ++) {
    mock_clock_ulong_millis::tick(rand() % 10);
    pull();
    if (value < 0.0f) {
      min_value = std::min(min_value, value);
    } else if (value > 0.0f) {
      max_value = std::max(max_value, value);
    }

    if (i % 800 == 0) {
      // After each phase, ensure that the signal wasn't attenuated,
      // then reset the min/max counter again.
      TEST_ASSERT_TRUE_MESSAGE(min_value < -2.0f && max_value > 2.0f, fmt::format("Low frequences shouldn't be attenuated; was between {} and {}", min_value, max_value).c_str());
      min_value = 0;
      max_value = 0;
    }
  }
}

void test_exponential_moving_average_responds_to_time_constant_change() {
  auto clock_source = sequence_open(1, 1);
  auto value_source = sine_wave(clock_source, constant(200), constant(0))
    | map([](float v) { return v * 10; });
  auto time_constant_source = MemoryState(400, false);
  auto avg = exponential_moving_average(value_source, clock_source, time_constant_source.get_source_fn());

  float value = 0.0f;
  auto pull = avg([&value](float v) { value = v; });

  for (int i = 0; i < 10000; i ++) {
    pull();
    TEST_ASSERT_TRUE_MESSAGE(abs(value) < 2.0f, fmt::format("Value {} should be between -2 and +2", value).c_str());
  }

  time_constant_source.set(100, false);
  float min_value = 0;
  float max_value = 0;
  for (int i = 1; i <= 10000; i ++) {
    pull();
    if (value < 0.0f) {
      min_value = std::min(min_value, value);
    } else if (value > 0.0f) {
      max_value = std::max(max_value, value);
    }

    if (i % 800 == 0) {
      // After each phase, ensure that the signal wasn't attenuated,
      // then reset the min/max counter again.
      TEST_ASSERT_TRUE_MESSAGE(min_value < -2.0f && max_value > 2.0f, fmt::format("Low frequences shouldn't be attenuated; was between {} and {}", min_value, max_value).c_str());
      min_value = 0;
      max_value = 0;
    }
  }
}

void test_exponential_moving_average_works_as_high_cut_and_low_pass() {
  auto clock_source = sequence_open(1, 1);
  auto period_source = MemoryState(100, false);
  auto value_source = sine_wave(clock_source, period_source.get_source_fn(false), constant(0))
    | map([](float v) { return v * 10; });
  auto time_constant_source = constant(400);
  auto avg = exponential_moving_average(value_source, clock_source, time_constant_source);

  float value = 0.0f;
  auto pull = avg([&value](float v) { value = v; });

  for (int i = 0; i < 10000; i ++) {
    pull();
    TEST_ASSERT_TRUE_MESSAGE(abs(value) < 2.0f, fmt::format("Value {} should be between -2 and +2", value).c_str());
  }

  period_source.set(800, false);
  float min_value = 0;
  float max_value = 0;
  for (int i = 1; i <= 10000; i ++) {
    pull();
    if (value < 0.0f) {
      min_value = std::min(min_value, value);
    } else if (value > 0.0f) {
      max_value = std::max(max_value, value);
    }

    if (i % 800 == 0) {
      // After each phase, ensure that the signal wasn't attenuated,
      // then reset the min/max counter again.
      TEST_ASSERT_TRUE_MESSAGE(min_value < -2.0f && max_value > 2.0f, fmt::format("Low frequences shouldn't be attenuated; was between {} and {}", min_value, max_value).c_str());
      min_value = 0;
      max_value = 0;
    }
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_exponential_moving_average_stays_stable);
  RUN_TEST(test_exponential_moving_average_accommodates_discontinuous_time_jumps);
  RUN_TEST(test_exponential_moving_average_responds_to_time_constant_change);
  RUN_TEST(test_exponential_moving_average_works_as_high_cut_and_low_pass);
  UNITY_END();
}
