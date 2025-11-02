#include <unity.h>
#include <functional>
#include <operators/interval.hpp>
#include <sources/fromClock.hpp>
#include <types/mock_clock.hpp>

using namespace rheo;
using namespace rheo::sources;
using namespace rheo::operators;

void test_interval_counts_correct_intervals() {
  mock_clock_ulong_millis::setTime(0);
  auto mockClock = fromClock<mock_clock_ulong_millis>();
  auto intervaled = interval(
    mockClock,
    every(mock_clock_ulong_millis::duration(100))
  );

  int intervalsCounted = 0;
  mock_clock_ulong_millis::time_point lastTimestamp;
  pull_fn pull = intervaled(
    [&intervalsCounted, &lastTimestamp](mock_clock_ulong_millis::time_point v) {
      intervalsCounted ++;
      lastTimestamp = v;
    }
  );

  for (int i = 0; i < 10; i ++) {
    for (int j = 0; j < 100; j ++) {
      pull();
      TEST_ASSERT_EQUAL_MESSAGE(i, intervalsCounted, "should have counted the right number of intervals");
      TEST_ASSERT_EQUAL_MESSAGE(i * 100, lastTimestamp.time_since_epoch().count(), "should have pushed the right timestamp");
      mock_clock_ulong_millis::tick();
    }
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_interval_counts_correct_intervals);
  UNITY_END();
}