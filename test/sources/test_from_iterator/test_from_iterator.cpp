#include <unity.h>
#include <sources/from_iterator.hpp>
#include <Endable.hpp>

using namespace rheo;
using namespace rheo::sources;

void test_from_iterator_yields_all_values_then_ends() {
  int pushed_value = 0;
  bool is_ended = false;
  std::vector<int> values { 1, 2, 3 };
  auto iter_over_numbers = from_iterator(values.begin(), values.end());
  auto pull = iter_over_numbers(
    [&pushed_value, &is_ended](Endable<int> v) {
      if (v.has_value()) {
        pushed_value = v.value();
      } else {
        is_ended = true;
      }
    }
  );
  for (int i = 1; i <= 3; i ++) {
    pull();
    TEST_ASSERT_FALSE_MESSAGE(is_ended, "shouldn't be ended until the last value");
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value, "should get next value in vector");
  }
  pull();
  TEST_ASSERT_TRUE_MESSAGE(is_ended, "should be done now");
}

void test_from_iterator_can_be_bound_to_twice_and_yield_twice() {
  int pushed_value1 = 0;
  bool is_ended1 = false;
  int pushed_value2 = 0;
  bool is_ended2 = false;
  std::vector<int> values { 1, 2, 3 };
  auto iter_over_numbers = from_iterator(values.begin(), values.end());
  auto pull1 = iter_over_numbers(
    [&pushed_value1, &is_ended1](Endable<int> v) {
      if (v.has_value()) {
        pushed_value1 = v.value();
      } else {
        is_ended1 = true;
      }
    }
  );
  auto pull2 = iter_over_numbers(
    [&pushed_value2, &is_ended2](Endable<int> v) {
      if (v.has_value()) {
        pushed_value2 = v.value();
      } else {
        is_ended2 = true;
      }
    }
  );
  for (int i = 1; i <= 3; i ++) {
    TEST_ASSERT_FALSE_MESSAGE(is_ended1, "first bound listener shouldn't be ended until the last value");
    pull1();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value1, "first bound listener should get next value in vector");
  }
  pull1();
  TEST_ASSERT_TRUE_MESSAGE(is_ended1, "first bound listener should be done now");
  for (int i = 1; i <= 3; i ++) {
    TEST_ASSERT_FALSE_MESSAGE(is_ended2, "second bound listener shouldn't be ended until the last value");
    pull2();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value2, "second bound listener should get next value in vector");
  }
  pull2();
  TEST_ASSERT_TRUE_MESSAGE(is_ended2, "second bound listener should be done now");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_from_iterator_yields_all_values_then_ends);
  RUN_TEST(test_from_iterator_can_be_bound_to_twice_and_yield_twice);
  UNITY_END();
}
