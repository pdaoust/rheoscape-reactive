#include <unity.h>
#include <functional>
#include <operators/stopwatch.hpp>
#include <operators/map.hpp>
#include <operators/unwrap.hpp>
#include <sources/constant.hpp>
#include <sources/from_iterator.hpp>
#include <sources/from_clock.hpp>
#include <types/mock_clock.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_stopwatch_is_contiguous_before_first_lap() {
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  //auto clock_source_as_duration = map<mock_clock_ulong_millis::duration, mock_clock_ulong_millis::time_point>(clock_source, [](mock_clock_ulong_millis::time_point v) { return v.time_since_epoch(); });
  std::vector<int> numbers { 0, -1, -2, 1, 2, 3, 0, 5, 6, 7, 0, 8, 9, 10 };
  auto numbers_source = unwrap_endable(from_iterator(numbers.begin(), numbers.end()));
  auto sw = stopwatch<mock_clock_ulong_millis::duration>(numbers_source, clock_source, [](auto v) { return v > 0; });

  mock_clock_ulong_millis::duration timestamp;
  int pushed_value;
  auto pull = sw(
    [&timestamp, &pushed_value](auto v) {
      timestamp = v.tag;
      pushed_value = v.value;
    }
  );
  for (int i = 0; i < 3; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, timestamp.count(), "Timestamp should be correct");
    TEST_ASSERT_EQUAL_MESSAGE(numbers[i], pushed_value, "Pushed value should be correct");
  }
}

void test_stopwatch_is_contiguous_through_first_lap() {
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  //auto clock_source_as_duration = map<mock_clock_ulong_millis::duration, mock_clock_ulong_millis::time_point>(clock_source, [](mock_clock_ulong_millis::time_point v) { return v.time_since_epoch(); });
  std::vector<int> numbers { 0, 1, 2, 3, 0, 5, 6, 7, 0, 8, 9, 10 };
  auto numbers_source = unwrap_endable(from_iterator(numbers.begin(), numbers.end()));
  auto sw = stopwatch<mock_clock_ulong_millis::duration>(numbers_source, clock_source, [](auto v) { return v > 0; });

  mock_clock_ulong_millis::duration timestamp;
  int pushed_value;
  auto pull = sw(
    [&timestamp, &pushed_value](auto v) {
      timestamp = v.tag;
      pushed_value = v.value;
    }
  );
  for (int i = 0; i < 5; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
  }

  for (int i = 5; i < 9; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i - 5, timestamp.count(), "Timestamp should be correct");
    TEST_ASSERT_EQUAL_MESSAGE(numbers[i], pushed_value, "Pushed value should be correct");
  }
}

void test_stopwatch_is_contiguous_through_second_lap() {
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  //auto clock_source_as_duration = map<mock_clock_ulong_millis::duration, mock_clock_ulong_millis::time_point>(clock_source, [](mock_clock_ulong_millis::time_point v) { return v.time_since_epoch(); });
  std::vector<int> numbers { 0, 1, 2, 3, 0, 5, 6, 7, 0, 8, 9, 10, 0 };
  auto numbers_source = unwrap_endable(from_iterator(numbers.begin(), numbers.end()));
  auto sw = stopwatch<mock_clock_ulong_millis::duration>(numbers_source, clock_source, [](auto v) { return v > 0; });

  mock_clock_ulong_millis::duration timestamp;
  int pushed_value;
  auto pull = sw(
    [&timestamp, &pushed_value](auto v) {
      timestamp = v.tag;
      pushed_value = v.value;
    }
  );
  for (int i = 0; i < 9; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
  }

  for (int i = 9; i < 13; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i - 9, timestamp.count(), "Timestamp should be correct");
    TEST_ASSERT_EQUAL_MESSAGE(numbers[i], pushed_value, "Pushed value should be correct");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_stopwatch_is_contiguous_before_first_lap);
  RUN_TEST(test_stopwatch_is_contiguous_through_first_lap);
  RUN_TEST(test_stopwatch_is_contiguous_through_second_lap);
  UNITY_END();
}