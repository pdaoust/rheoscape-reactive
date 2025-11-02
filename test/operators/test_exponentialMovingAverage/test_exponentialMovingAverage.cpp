#include <unity.h>
#include <algorithm>
#include <functional>
#include <operators/exponentialMovingAverage.hpp>
#include <operators/map.hpp>
#include <operators/waves.hpp>
#include <sources/constant.hpp>
#include <sources/fromClock.hpp>
#include <sources/sequence.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <fmt/format.h>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_exponentialMovingAverage_stays_stable() {
  // We're going to make it as simple as possible
  // and use an integer counter for the clock source.
  auto clockSource = sequenceOpen(1, 1);
  auto valueSource = constant(5);
  auto timeConstantSource = constant(10);
  auto avg = exponentialMovingAverage(valueSource, clockSource, timeConstantSource);

  int value = 0;
  int pushCount = 0;
  auto pull = avg([&value, &pushCount](int v) {
    value = v;
    pushCount ++;
  });

  for (int i = 1; i <= 1000; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushCount, "Should have pushed");
    TEST_ASSERT_EQUAL_MESSAGE(5, value, "Value should stay stable");
  }
}

void test_exponentialMovingAverage_accommodates_discontinuous_time_jumps() {
  auto clockSource = fromClock<mock_clock_ulong_millis>()
    | map([](mock_clock_ulong_millis::time_point v) { return v.time_since_epoch().count(); });
  auto periodSource = State(100UL, false);
  auto valueSource = sineWave(clockSource, periodSource.sourceFn(), constant(0UL))
    | map([](float v) { return v * 10; });
  auto timeConstantSource = constant(400UL);
  auto avg = exponentialMovingAverage(valueSource, clockSource, timeConstantSource);

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

  periodSource.set(800, false);
  float minValue = 0;
  float maxValue = 0;
  for (int i = 1; i <= 10000; i ++) {
    mock_clock_ulong_millis::tick(rand() % 10);
    pull();
    if (value < 0.0f) {
      minValue = std::min(minValue, value);
    } else if (value > 0.0f) {
      maxValue = std::max(maxValue, value);
    }

    if (i % 800 == 0) {
      // After each phase, ensure that the signal wasn't attenuated,
      // then reset the min/max counter again.
      TEST_ASSERT_TRUE_MESSAGE(minValue < -2.0f && maxValue > 2.0f, fmt::format("Low frequences shouldn't be attenuated; was between {} and {}", minValue, maxValue).c_str());
      minValue = 0;
      maxValue = 0;
    }
  }
}

void test_exponentialMovingAverage_responds_to_time_constant_change() {
  auto clockSource = sequenceOpen(1, 1);
  auto valueSource = sineWave(clockSource, constant(200), constant(0))
    | map([](float v) { return v * 10; });
  auto timeConstantSource = State(400, false);
  auto avg = exponentialMovingAverage(valueSource, clockSource, timeConstantSource.sourceFn());

  float value = 0.0f;
  auto pull = avg([&value](float v) { value = v; });

  for (int i = 0; i < 10000; i ++) {
    pull();
    TEST_ASSERT_TRUE_MESSAGE(abs(value) < 2.0f, fmt::format("Value {} should be between -2 and +2", value).c_str());
  }

  timeConstantSource.set(100, false);
  float minValue = 0;
  float maxValue = 0;
  for (int i = 1; i <= 10000; i ++) {
    pull();
    if (value < 0.0f) {
      minValue = std::min(minValue, value);
    } else if (value > 0.0f) {
      maxValue = std::max(maxValue, value);
    }

    if (i % 800 == 0) {
      // After each phase, ensure that the signal wasn't attenuated,
      // then reset the min/max counter again.
      TEST_ASSERT_TRUE_MESSAGE(minValue < -2.0f && maxValue > 2.0f, fmt::format("Low frequences shouldn't be attenuated; was between {} and {}", minValue, maxValue).c_str());
      minValue = 0;
      maxValue = 0;
    }
  }
}

void test_exponentialMovingAverage_works_as_high_cut_and_low_pass() {
  auto clockSource = sequenceOpen(1, 1);
  auto periodSource = State(100, false);
  auto valueSource = sineWave(clockSource, periodSource.sourceFn(false), constant(0))
    | map([](float v) { return v * 10; });
  auto timeConstantSource = constant(400);
  auto avg = exponentialMovingAverage(valueSource, clockSource, timeConstantSource);

  float value = 0.0f;
  auto pull = avg([&value](float v) { value = v; });

  for (int i = 0; i < 10000; i ++) {
    pull();
    TEST_ASSERT_TRUE_MESSAGE(abs(value) < 2.0f, fmt::format("Value {} should be between -2 and +2", value).c_str());
  }

  periodSource.set(800, false);
  float minValue = 0;
  float maxValue = 0;
  for (int i = 1; i <= 10000; i ++) {
    pull();
    if (value < 0.0f) {
      minValue = std::min(minValue, value);
    } else if (value > 0.0f) {
      maxValue = std::max(maxValue, value);
    }

    if (i % 800 == 0) {
      // After each phase, ensure that the signal wasn't attenuated,
      // then reset the min/max counter again.
      TEST_ASSERT_TRUE_MESSAGE(minValue < -2.0f && maxValue > 2.0f, fmt::format("Low frequences shouldn't be attenuated; was between {} and {}", minValue, maxValue).c_str());
      minValue = 0;
      maxValue = 0;
    }
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_exponentialMovingAverage_stays_stable);
  RUN_TEST(test_exponentialMovingAverage_accommodates_discontinuous_time_jumps);
  RUN_TEST(test_exponentialMovingAverage_responds_to_time_constant_change);
  RUN_TEST(test_exponentialMovingAverage_works_as_high_cut_and_low_pass);
  UNITY_END();
}
