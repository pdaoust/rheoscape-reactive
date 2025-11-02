#include <unity.h>
#include <sources/fromIterator.hpp>
#include <Endable.hpp>

using namespace rheo;
using namespace rheo::sources;

void test_fromIterator_yields_all_values_then_ends() {
  int pushedValue = 0;
  bool isEnded = false;
  std::vector<int> values { 1, 2, 3 };
  auto iterOverNumbers = fromIterator(values.begin(), values.end());
  auto pull = iterOverNumbers(
    [&pushedValue, &isEnded](Endable<int> v) {
      if (v.hasValue()) {
        pushedValue = v.value();
      } else {
        isEnded = true;
      }
    }
  );
  for (int i = 1; i <= 3; i ++) {
    pull();
    TEST_ASSERT_FALSE_MESSAGE(isEnded, "shouldn't be ended until the last value");
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "should get next value in vector");
  }
  pull();
  TEST_ASSERT_TRUE_MESSAGE(isEnded, "should be done now");
}

void test_fromIterator_can_be_bound_to_twice_and_yield_twice() {
  int pushedValue1 = 0;
  bool isEnded1 = false;
  int pushedValue2 = 0;
  bool isEnded2 = false;
  std::vector<int> values { 1, 2, 3 };
  auto iterOverNumbers = fromIterator(values.begin(), values.end());
  auto pull1 = iterOverNumbers(
    [&pushedValue1, &isEnded1](Endable<int> v) {
      if (v.hasValue()) {
        pushedValue1 = v.value();
      } else {
        isEnded1 = true;
      }
    }
  );
  auto pull2 = iterOverNumbers(
    [&pushedValue2, &isEnded2](Endable<int> v) {
      if (v.hasValue()) {
        pushedValue2 = v.value();
      } else {
        isEnded2 = true;
      }
    }
  );
  for (int i = 1; i <= 3; i ++) {
    TEST_ASSERT_FALSE_MESSAGE(isEnded1, "first bound listener shouldn't be ended until the last value");
    pull1();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue1, "first bound listener should get next value in vector");
  }
  pull1();
  TEST_ASSERT_TRUE_MESSAGE(isEnded1, "first bound listener should be done now");
  for (int i = 1; i <= 3; i ++) {
    TEST_ASSERT_FALSE_MESSAGE(isEnded2, "second bound listener shouldn't be ended until the last value");
    pull2();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue2, "second bound listener should get next value in vector");
  }
  pull2();
  TEST_ASSERT_TRUE_MESSAGE(isEnded2, "second bound listener should be done now");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_fromIterator_yields_all_values_then_ends);
  RUN_TEST(test_fromIterator_can_be_bound_to_twice_and_yield_twice);
  UNITY_END();
}
