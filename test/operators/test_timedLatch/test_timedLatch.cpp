#include <unity.h>
#include <functional>
#include <operators/timedLatch.hpp>
#include <sources/fromClock.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <fmt/format.h>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_timedLatch_latches_simple_state() {
  mock_clock_ulong_millis::setTime(0);
  auto clockSource = fromClock<mock_clock_ulong_millis>();
  auto valueSource = State(false, false);
  auto latchedSource = timedLatch(valueSource.sourceFn(false), clockSource, mock_clock_ulong_millis::duration(10), false);
  bool value = false;
  int pushedCount = 0;
  pull_fn pull = latchedSource([&value, &pushedCount](bool v) {
    value = v;
    pushedCount ++;
  });
  pull();
  TEST_ASSERT_FALSE_MESSAGE(value, "Initial value should be false");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should have pushed once");

  mock_clock_ulong_millis::tick();
  valueSource.set(true, false);
  pull();

  for (int i = 1; i <= 10; i ++) {
    TEST_ASSERT_TRUE_MESSAGE(value, "Value should be true during latch interval");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should have pushed");
    mock_clock_ulong_millis::tick();
    valueSource.set(false, false);
    pull();
  }

  TEST_ASSERT_FALSE_MESSAGE(value, "Value should return to default state");
}

void test_timedLatch_latches_simple_state_when_using_push_source() {
  mock_clock_ulong_millis::setTime(0);
  auto clockSource = fromClock<mock_clock_ulong_millis>();
  auto valueSource = State(false, true);
  auto latchedSource = timedLatch(valueSource.sourceFn(true), clockSource, mock_clock_ulong_millis::duration(10), false);
  bool value = false;
  int pushedCount = 0;
  pull_fn pull = latchedSource([&value, &pushedCount](bool v) {
    value = v;
    pushedCount ++;
  });
  TEST_ASSERT_FALSE_MESSAGE(value, "Initial value should be false");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should have pushed once");

  mock_clock_ulong_millis::tick();
  valueSource.set(true, true);

  for (int i = 1; i <= 10; i ++) {
    TEST_ASSERT_TRUE_MESSAGE(value, "Value should be true during latch interval");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should have pushed");
    mock_clock_ulong_millis::tick();
    valueSource.set(false, true);
  }

  TEST_ASSERT_FALSE_MESSAGE(value, "Value should return to default state");
}

void test_timedLatch_handles_complex_state() {
  mock_clock_ulong_millis::setTime(0);
  auto clockSource = fromClock<mock_clock_ulong_millis>();
  auto valueSource = State(0, false);
  auto latchedSource = timedLatch(valueSource.sourceFn(false), clockSource, mock_clock_ulong_millis::duration(10), 0);
  int value = INT_MAX;
  int pushedCount = 0;
  pull_fn pull = latchedSource([&value, &pushedCount](int v) {
    value = v;
    pushedCount ++;
  });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, value, "Initial value should be 0");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should have pushed once");

  mock_clock_ulong_millis::tick();
  valueSource.set(4, false);
  pull();

  for (int i = 1; i <= 10; i ++) {
    TEST_ASSERT_EQUAL_MESSAGE(4, value, "Value should be 4 during latch interval");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should have pushed");
    mock_clock_ulong_millis::tick();
    valueSource.set(0, false);
    pull();
  }

  TEST_ASSERT_EQUAL_MESSAGE(0, value, "Value should return to default state");

  // Now let's test that it picks up a new non-default state
  // that comes through halfway through the period.
  mock_clock_ulong_millis::tick();
  valueSource.set(2, false);
  pull();

  for (int i = 1; i <= 10; i ++) {
    TEST_ASSERT_EQUAL_MESSAGE(2, value, "Value should be 2 during latch interval");
    mock_clock_ulong_millis::tick();
    if (i == 5) {
      valueSource.set(-3, false);
    }
    pull();
  }

  for (int i = 1; i <= 10; i ++) {
    TEST_ASSERT_EQUAL_MESSAGE(-3, value, "Value should be -3 in a new interval");
    mock_clock_ulong_millis::tick();
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_timedLatch_latches_simple_state);
  RUN_TEST(test_timedLatch_latches_simple_state_when_using_push_source);
  RUN_TEST(test_timedLatch_handles_complex_state);
  UNITY_END();
}