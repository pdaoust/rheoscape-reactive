#include <unity.h>
#include <functional>
#include <operators/throttle.hpp>
#include <sources/fromClock.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;


void test_throttle_throttles() {
  auto clockSource = fromClock<mock_clock_ulong_millis>();
  State<int> valueSource(0);
  auto throttled = throttle(valueSource.sourceFn(), clockSource, mock_clock_ulong_millis::duration(10));

  int pushedCount = 0;
  int pushedValue = 0;
  throttled(
    [&pushedCount, &pushedValue](int value) { pushedCount ++; pushedValue = value; }
  );

  valueSource.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should push at beginning");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "Should have pushed the right value");
  for (int i = 2; i < 12; i ++) {
    mock_clock_ulong_millis::tick();
    valueSource.set(i);
    TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should not have pushed anything new in the throttle period");
    TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "Should still have the same old value");
  }
  mock_clock_ulong_millis::tick();
  valueSource.set(11);
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedCount, "Should push after throttle period ends");
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedValue, "Should have pushed the right value");
  mock_clock_ulong_millis::tick(11);
  valueSource.set(12);
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedCount, "Should not snap to throttle period");
  TEST_ASSERT_EQUAL_MESSAGE(12, pushedValue, "Should have pushed the right value");
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_throttle_throttles);
  UNITY_END();
}