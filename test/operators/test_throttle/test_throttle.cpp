#include <unity.h>
#include <functional>
#include <operators/throttle.hpp>
#include <sources/from_clock.hpp>
#include <types/mock_clock.hpp>
#include <states/MemoryState.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;


void test_throttle_throttles() {
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  MemoryState<int> value_source(0);
  auto throttled = throttle(value_source.get_source_fn(), clock_source, mock_clock_ulong_millis::duration(10));

  int pushed_count = 0;
  int pushed_value = 0;
  throttled(
    [&pushed_count, &pushed_value](int value) { pushed_count ++; pushed_value = value; }
  );

  value_source.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should push at beginning");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value, "Should have pushed the right value");
  for (int i = 2; i < 12; i ++) {
    mock_clock_ulong_millis::tick();
    value_source.set(i);
    TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should not have pushed anything new in the throttle period");
    TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value, "Should still have the same old value");
  }
  mock_clock_ulong_millis::tick();
  value_source.set(11);
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_count, "Should push after throttle period ends");
  TEST_ASSERT_EQUAL_MESSAGE(11, pushed_value, "Should have pushed the right value");
  mock_clock_ulong_millis::tick(11);
  value_source.set(12);
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_count, "Should not snap to throttle period");
  TEST_ASSERT_EQUAL_MESSAGE(12, pushed_value, "Should have pushed the right value");
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_throttle_throttles);
  UNITY_END();
}