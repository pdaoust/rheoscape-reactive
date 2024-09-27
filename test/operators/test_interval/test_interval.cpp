#include <unity.h>
#include <functional>
#include <operators/interval.hpp>
#include <sources/fromClock.hpp>
#include <types/mock_clock.hpp>

void test_interval_counts_correct_intervals() {
  rheo::mock_clock_ulong_millis::setTime(0);
  auto mockClock = rheo::fromClock<rheo::mock_clock_ulong_millis>();
  auto interval = rheo::interval(
    mockClock,
    rheo::every(rheo::mock_clock_ulong_millis::duration(100))
  );

  int intervalsCounted = 0;
  rheo::mock_clock_ulong_millis::time_point lastTimestamp;
  rheo::pull_fn pull = interval(
    [&intervalsCounted, &lastTimestamp](rheo::mock_clock_ulong_millis::time_point v) {
      intervalsCounted ++;
      lastTimestamp = v;
    },
    [](){}
  );

  for (int i = 0; i < 10; i ++) {
    for (int j = 0; j < 100; j ++) {
      pull();
      TEST_ASSERT_EQUAL_MESSAGE(i, intervalsCounted, "should have counted the right number of intervals");
      TEST_ASSERT_EQUAL_MESSAGE(i * 100, lastTimestamp.time_since_epoch().count(), "should have pushed the right timestamp");
      rheo::mock_clock_ulong_millis::tick();
    }
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_interval_counts_correct_intervals);
  UNITY_END();
}