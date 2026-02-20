#include <unity.h>
#include <functional>
#include <operators/timestamp.hpp>
#include <operators/unwrap.hpp>
#include <sources/from_clock.hpp>
#include <sources/sequence.hpp>
#include <types/mock_clock.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_timestamp_timestamps() {
  auto numbers_source = unwrap_endable(sequence(0, 10, 1));
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  auto timestamped = timestamp(numbers_source, clock_source);
  int pushed_value = -1;
  mock_clock_ulong_millis::time_point timestamp;
  auto pull = timestamped(
    [&pushed_value, &timestamp](std::tuple<int, mock_clock_ulong_millis::time_point> value) {
      pushed_value = value.value;
      timestamp = value.tag;
    }
  );

  for (int i = 0; i <= 10; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value, "Should push a value");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, timestamp.time_since_epoch().count(), "should push the right timestamp");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_timestamp_timestamps);
  UNITY_END();
}