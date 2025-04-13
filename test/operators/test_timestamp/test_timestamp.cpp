#include <unity.h>
#include <functional>
#include <operators/timestamp.hpp>
#include <sources/fromClock.hpp>
#include <sources/sequence.hpp>
#include <types/mock_clock.hpp>

void test_timestamp_timestamps() {
  auto numbersSource = rheo::sequence(0, 10, 1);
  auto clockSource = rheo::fromClock<rheo::mock_clock_ulong_millis>();
  auto timestamped = timestamp(numbersSource, clockSource);
  int pushedValue = -1;
  rheo::mock_clock_ulong_millis::time_point timestamp;
  auto pull = timestamped(
    [&pushedValue, &timestamp](rheo::TaggedValue<int, rheo::mock_clock_ulong_millis::time_point> value) {
      pushedValue = value.value;
      timestamp = value.tag;
    },
    [](){}
  );

  for (int i = 0; i <= 10; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "Should push a value");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, timestamp.time_since_epoch().count(), "should push the right timestamp");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_timestamp_timestamps);
  UNITY_END();
}