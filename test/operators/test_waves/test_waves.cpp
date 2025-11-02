#include <unity.h>
#include <functional>
#include <operators/waves.hpp>
#include <sources/constant.hpp>
#include <sources/sequence.hpp>
#include <types/State.hpp>
#include <fmt/format.h>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;


void test_wave_waves() {
  auto inputSource = sequenceOpen(0, 1);
  auto periodSource = constant(100);
  auto phaseShiftSource = constant(0);
  auto waveSource = wave(inputSource, periodSource, phaseShiftSource, [](float value) { return value; });

  float pushedValue = 0;
  auto pull = waveSource([&pushedValue](float v) { pushedValue = v; });

  for (int i = 0; i < 200; i ++) {
    pull();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE((float)(i % 100) / 100, pushedValue, "Should calculate normalised phase");
    if (i % 100 == 0) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pushedValue, "Should pass zero phase smoke test");
    } else if (i % 100 == 25) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.25f, pushedValue, "Should pass quarter phase smoke test");
    } else if (i % 100 == 50) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.5f, pushedValue, "Should pass half phase smoke test");
    } else if (i % 100 == 75) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.75f, pushedValue, "Should pass three quarters phase smoke test");
    }
  }
}

void test_wave_shifts_phase() {
  auto inputSource = sequenceOpen(0, 1);
  auto periodSource = constant(100);
  auto phaseShiftSource = constant(25);
  auto waveSource = wave(inputSource, periodSource, phaseShiftSource, [](float value) { return value; });

  float pushedValue = 0;
  auto pull = waveSource([&pushedValue](float v) { pushedValue = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    if (i < 75) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE((float)i / 100 + 0.25f, pushedValue, "Should calculate normalised phase value");
    } else {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE((float)i / 100 - 0.75f, pushedValue, "Should calculate normalised phase value");
    }
  }
}

void test_sine_wave_waves() {
  auto inputSource = sequenceOpen(0, 1);
  auto periodSource = constant(100);
  auto phaseShiftSource = constant(0);
  auto sineSource = sineWave(inputSource, periodSource, phaseShiftSource);

  float pushedValue = 0;
  auto pull = sineSource([&pushedValue](float v) { pushedValue = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    if (i == 0 || i == 50) {
      // sin(pi) != 0
      TEST_ASSERT_TRUE_MESSAGE(pushedValue < 0.000000001f, "Should pass zero crossing smoke test");
    } else if (i == 25) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, pushedValue, "Should pass trough smoke test");
    } else if (i == 75) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-1.0f, pushedValue, "Should pass peak smoke test");
    } else {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(sin((float)i / 100 * M_PI * 2), pushedValue, "Should calculate sin");
    }
  }
}

void test_square_wave_waves() {
  auto inputSource = sequenceOpen(0, 1);
  auto periodSource = constant(100);
  auto phaseShiftSource = constant(0);
  auto squareSource = squareWave(inputSource, periodSource, phaseShiftSource);

  float pushedValue = 0;
  int pushedCount = 0;
  auto pull = squareSource([&pushedValue, &pushedCount](float v) { pushedCount ++; pushedValue = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should have pushed the right number of times");
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(i < 50 ? 1.0f : -1.0f, pushedValue, std::format("Should calculate square wave correctly at {}", i).c_str());
  }
}

void test_sawtooth_wave_waves() {
  auto inputSource = sequenceOpen(0, 1);
  auto periodSource = constant(100);
  auto phaseShiftSource = constant(0);
  auto sawtoothSource = sawtoothWave(inputSource, periodSource, phaseShiftSource);

  float pushedValue = 0;
  auto pull = sawtoothSource([&pushedValue](float v) { pushedValue = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    if (i == 0) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-1.0f, pushedValue, "Should pass start smoke test");
    } else if (i == 50) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pushedValue, "Should pass zero crossing smoke test");
    } else if (i == 99) {
      TEST_ASSERT_TRUE_MESSAGE(pushedValue >= 0.98f, "Should pass end smoke test");
    }
  }
}

void test_triangle_wave_waves() {
  auto inputSource = sequenceOpen(0, 1);
  auto periodSource = constant(100);
  auto phaseShiftSource = constant(0);
  auto triangleSource = triangleWave(inputSource, periodSource, phaseShiftSource);

  float pushedValue = 0;
  auto pull = triangleSource([&pushedValue](float v) { pushedValue = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    if (i == 0 || i == 50) {
      // sin(pi) != 0
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, pushedValue, "Should pass zero crossing smoke test");
    } else if (i == 25) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0f, pushedValue, "Should pass peak smoke test");
    } else if (i == 75) {
      TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-1.0f, pushedValue, "Should pass trough smoke test");
    }
  }
}

void test_pwm_wave_waves() {
  auto inputSource = sequenceOpen(0, 1);
  auto periodSource = constant(100);
  auto phaseShiftSource = constant(0);
  State<float> dutyState(0.0f, false);
  auto pwmSource = pwmWave(inputSource, periodSource, phaseShiftSource, dutyState.sourceFn(false));

  float pushedValue;
  auto pull = pwmSource([&pushedValue](auto v) { pushedValue = v; });

  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-1.0f, pushedValue, fmt::format("Should calculate PWM wave with 0 duty at {}", i).c_str());
  }

  dutyState.set(0.25f, false);
  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(i < 25 ? 1.0f : -1.0f, pushedValue, fmt::format("Should calculate PWM wave with 0.25 duty at {}", i).c_str());
  }

  dutyState.set(0.75f, false);
  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(i < 75 ? 1.0f : -1.0f, pushedValue, fmt::format("Should calculate PWM wave with 0.75 duty at {}", i).c_str());
  }

  dutyState.set(1.0f, false);
  for (int i = 0; i < 100; i ++) {
    pull();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(1.0, pushedValue, "Should calculate PWM wave with 1.0 duty");
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
  UNITY_END();
}