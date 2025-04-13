#include <unity.h>
#include <functional>
#include <operators/stopwatch.hpp>
#include <operators/map.hpp>
#include <sources/constant.hpp>
#include <sources/fromIterator.hpp>
#include <sources/fromClock.hpp>
#include <types/mock_clock.hpp>

void test_stopwatch_is_contiguous_before_first_lap() {
  auto clockSource = rheo::fromClock<rheo::mock_clock_ulong_millis>();
  //auto clockSourceAsDuration = rheo::map<rheo::mock_clock_ulong_millis::duration, rheo::mock_clock_ulong_millis::time_point>(clockSource, [](rheo::mock_clock_ulong_millis::time_point v) { return v.time_since_epoch(); });
  std::vector<int> numbers { 0, -1, -2, 1, 2, 3, 0, 5, 6, 7, 0, 8, 9, 10 };
  auto numbersSource = rheo::fromIterator(numbers.begin(), numbers.end());
  auto stopwatch = rheo::stopwatch<rheo::mock_clock_ulong_millis::duration>(numbersSource, clockSource, (rheo::filter_fn<int>)[](int v) { return v > 0; });

  rheo::mock_clock_ulong_millis::duration timestamp;
  int pushedValue;
  auto pull = stopwatch(
    [&timestamp, &pushedValue](rheo::TaggedValue<int, rheo::mock_clock_ulong_millis::duration> v) {
      timestamp = v.tag;
      pushedValue = v.value;
    },
    [](){}
  );
  for (int i = 0; i < 3; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, timestamp.count(), "Timestamp should be correct");
    TEST_ASSERT_EQUAL_MESSAGE(numbers[i], pushedValue, "Pushed value should be correct");
  }
}

void test_stopwatch_is_contiguous_through_first_lap() {
  auto clockSource = rheo::fromClock<rheo::mock_clock_ulong_millis>();
  //auto clockSourceAsDuration = rheo::map<rheo::mock_clock_ulong_millis::duration, rheo::mock_clock_ulong_millis::time_point>(clockSource, [](rheo::mock_clock_ulong_millis::time_point v) { return v.time_since_epoch(); });
  std::vector<int> numbers { 0, 1, 2, 3, 0, 5, 6, 7, 0, 8, 9, 10 };
  auto numbersSource = rheo::fromIterator(numbers.begin(), numbers.end());
  auto stopwatch = rheo::stopwatch<rheo::mock_clock_ulong_millis::duration>(numbersSource, clockSource, (rheo::filter_fn<int>)[](int v) { return v > 0; });

  rheo::mock_clock_ulong_millis::duration timestamp;
  int pushedValue;
  auto pull = stopwatch(
    [&timestamp, &pushedValue](rheo::TaggedValue<int, rheo::mock_clock_ulong_millis::duration> v) {
      timestamp = v.tag;
      pushedValue = v.value;
    },
    [](){}
  );
  for (int i = 0; i < 5; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
  }

  for (int i = 5; i < 9; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i - 5, timestamp.count(), "Timestamp should be correct");
    TEST_ASSERT_EQUAL_MESSAGE(numbers[i], pushedValue, "Pushed value should be correct");
  }
}

void test_stopwatch_is_contiguous_through_second_lap() {
  auto clockSource = rheo::fromClock<rheo::mock_clock_ulong_millis>();
  //auto clockSourceAsDuration = rheo::map<rheo::mock_clock_ulong_millis::duration, rheo::mock_clock_ulong_millis::time_point>(clockSource, [](rheo::mock_clock_ulong_millis::time_point v) { return v.time_since_epoch(); });
  std::vector<int> numbers { 0, 1, 2, 3, 0, 5, 6, 7, 0, 8, 9, 10, 0 };
  auto numbersSource = rheo::fromIterator(numbers.begin(), numbers.end());
  auto stopwatch = rheo::stopwatch<rheo::mock_clock_ulong_millis::duration>(numbersSource, clockSource, (rheo::filter_fn<int>)[](int v) { return v > 0; });

  rheo::mock_clock_ulong_millis::duration timestamp;
  int pushedValue;
  auto pull = stopwatch(
    [&timestamp, &pushedValue](rheo::TaggedValue<int, rheo::mock_clock_ulong_millis::duration> v) {
      timestamp = v.tag;
      pushedValue = v.value;
    },
    [](){}
  );
  for (int i = 0; i < 9; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
  }

  for (int i = 9; i < 13; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i - 9, timestamp.count(), "Timestamp should be correct");
    TEST_ASSERT_EQUAL_MESSAGE(numbers[i], pushedValue, "Pushed value should be correct");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_stopwatch_is_contiguous_before_first_lap);
  RUN_TEST(test_stopwatch_is_contiguous_through_first_lap);
  RUN_TEST(test_stopwatch_is_contiguous_through_second_lap);
  UNITY_END();
}