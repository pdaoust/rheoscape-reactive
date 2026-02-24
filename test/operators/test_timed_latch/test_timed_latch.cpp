#include <unity.h>
#include <functional>
#include <operators/timed_latch.hpp>
#include <sources/from_clock.hpp>
#include <types/mock_clock.hpp>
#include <states/MemoryState.hpp>
#include <fmt/format.h>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;
using namespace rheoscape::states;

void test_timed_latch_latches_simple_state() {
  mock_clock_ulong_millis::set_time(0);
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  auto value_source = MemoryState(false, false);
  auto latched_source = timed_latch(value_source.get_source_fn(false), clock_source, mock_clock_ulong_millis::duration(10), false);
  bool value = false;
  int pushed_count = 0;
  pull_fn pull = latched_source([&value, &pushed_count](bool v) {
    value = v;
    pushed_count ++;
  });
  pull();
  TEST_ASSERT_FALSE_MESSAGE(value, "Initial value should be false");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should have pushed once");

  mock_clock_ulong_millis::tick();
  value_source.set(true, false);
  pull();

  for (int i = 1; i <= 10; i ++) {
    TEST_ASSERT_TRUE_MESSAGE(value, "Value should be true during latch interval");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushed_count, "Should have pushed");
    mock_clock_ulong_millis::tick();
    value_source.set(false, false);
    pull();
  }

  TEST_ASSERT_FALSE_MESSAGE(value, "Value should return to default state");
}

void test_timed_latch_latches_simple_state_when_using_push_source() {
  mock_clock_ulong_millis::set_time(0);
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  auto value_source = MemoryState(false, true);
  auto latched_source = timed_latch(value_source.get_source_fn(true), clock_source, mock_clock_ulong_millis::duration(10), false);
  bool value = false;
  int pushed_count = 0;
  pull_fn pull = latched_source([&value, &pushed_count](bool v) {
    value = v;
    pushed_count ++;
  });
  TEST_ASSERT_FALSE_MESSAGE(value, "Initial value should be false");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should have pushed once");

  mock_clock_ulong_millis::tick();
  value_source.set(true, true);

  for (int i = 1; i <= 10; i ++) {
    TEST_ASSERT_TRUE_MESSAGE(value, "Value should be true during latch interval");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushed_count, "Should have pushed");
    mock_clock_ulong_millis::tick();
    value_source.set(false, true);
  }

  TEST_ASSERT_FALSE_MESSAGE(value, "Value should return to default state");
}

void test_timed_latch_handles_complex_state() {
  mock_clock_ulong_millis::set_time(0);
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  auto value_source = MemoryState(0, false);
  auto latched_source = timed_latch(value_source.get_source_fn(false), clock_source, mock_clock_ulong_millis::duration(10), 0);
  int value = INT_MAX;
  int pushed_count = 0;
  pull_fn pull = latched_source([&value, &pushed_count](int v) {
    value = v;
    pushed_count ++;
  });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, value, "Initial value should be 0");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should have pushed once");

  mock_clock_ulong_millis::tick();
  value_source.set(4, false);
  pull();

  for (int i = 1; i <= 10; i ++) {
    TEST_ASSERT_EQUAL_MESSAGE(4, value, "Value should be 4 during latch interval");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushed_count, "Should have pushed");
    mock_clock_ulong_millis::tick();
    value_source.set(0, false);
    pull();
  }

  TEST_ASSERT_EQUAL_MESSAGE(0, value, "Value should return to default state");

  // Now let's test that it picks up a new non-default state
  // that comes through halfway through the period.
  mock_clock_ulong_millis::tick();
  value_source.set(2, false);
  pull();

  for (int i = 1; i <= 10; i ++) {
    TEST_ASSERT_EQUAL_MESSAGE(2, value, "Value should be 2 during latch interval");
    mock_clock_ulong_millis::tick();
    if (i == 5) {
      value_source.set(-3, false);
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
  RUN_TEST(test_timed_latch_latches_simple_state);
  RUN_TEST(test_timed_latch_latches_simple_state_when_using_push_source);
  RUN_TEST(test_timed_latch_handles_complex_state);
  UNITY_END();
}