#include <unity.h>
#include <functional>
#include <operators/interval.hpp>
#include <sources/from_clock.hpp>
#include <types/mock_clock.hpp>

using namespace rheoscape;
using namespace rheoscape::sources;
using namespace rheoscape::operators;

void test_interval_counts_correct_intervals() {
  mock_clock_ulong_millis::set_time(0);
  auto mock_clock = from_clock<mock_clock_ulong_millis>();
  auto intervaled = interval(
    mock_clock,
    every(mock_clock_ulong_millis::duration(100))
  );

  int intervals_counted = 0;
  mock_clock_ulong_millis::time_point last_timestamp;
  pull_fn pull = intervaled(
    [&intervals_counted, &last_timestamp](mock_clock_ulong_millis::time_point v) {
      intervals_counted ++;
      last_timestamp = v;
    }
  );

  for (int i = 0; i < 10; i ++) {
    for (int j = 0; j < 100; j ++) {
      pull();
      TEST_ASSERT_EQUAL_MESSAGE(i, intervals_counted, "should have counted the right number of intervals");
      TEST_ASSERT_EQUAL_MESSAGE(i * 100, last_timestamp.time_since_epoch().count(), "should have pushed the right timestamp");
      mock_clock_ulong_millis::tick();
    }
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_interval_counts_correct_intervals);
  UNITY_END();
}