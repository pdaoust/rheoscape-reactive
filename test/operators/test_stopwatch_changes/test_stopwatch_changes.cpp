#include <unity.h>
#include <functional>
#include <operators/stopwatch_changes.hpp>
#include <operators/map.hpp>
#include <operators/unwrap.hpp>
#include <sources/constant.hpp>
#include <sources/from_iterator.hpp>
#include <sources/from_clock.hpp>
#include <types/mock_clock.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_stopwatch_is_contiguous_through_first_lap() {
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  std::vector<int> numbers { 0, 0, 0, 1, 1, 2, 2, 2, 2, 10, 11, 2, 2, 2 };
  auto numbers_source = unwrap_endable(from_iterator(numbers.begin(), numbers.end()));
  auto sw = stopwatch_changes<mock_clock_ulong_millis::duration>(numbers_source, clock_source);

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

void test_stopwatch_is_contiguous_through_second_lap() {
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  std::vector<int> numbers { 0, 0, 0, 1, 1, 2, 2, 2, 2, 10, 11, 2, 2, 2 };
  auto numbers_source = unwrap_endable(from_iterator(numbers.begin(), numbers.end()));
  auto sw = stopwatch_changes<mock_clock_ulong_millis::duration>(numbers_source, clock_source);

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
  }

  for (int i = 3; i < 5; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i - 3, timestamp.count(), "Timestamp should be correct");
    TEST_ASSERT_EQUAL_MESSAGE(numbers[i], pushed_value, "Pushed value should be correct");
  }
}

void test_stopwatch_doesnt_resume_prior_lap() {
  auto clock_source = from_clock<mock_clock_ulong_millis>();
  std::vector<int> numbers { 0, 0, 0, 1, 1, 2, 2, 2, 2, 10, 11, 2, 2, 2 };
  auto numbers_source = unwrap_endable(from_iterator(numbers.begin(), numbers.end()));
  auto sw = stopwatch_changes<mock_clock_ulong_millis::duration>(numbers_source, clock_source);

  mock_clock_ulong_millis::duration timestamp;
  int pushed_value;
  auto pull = sw(
    [&timestamp, &pushed_value](auto v) {
      timestamp = v.tag;
      pushed_value = v.value;
    }
  );
  for (int i = 0; i < 11; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
  }

  for (int i = 11; i < 14; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i - 11, timestamp.count(), "Timestamp should be correct");
    TEST_ASSERT_EQUAL_MESSAGE(numbers[i], pushed_value, "Pushed value should be correct");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_stopwatch_is_contiguous_through_first_lap);
  RUN_TEST(test_stopwatch_is_contiguous_through_second_lap);
  RUN_TEST(test_stopwatch_doesnt_resume_prior_lap);
  UNITY_END();
}