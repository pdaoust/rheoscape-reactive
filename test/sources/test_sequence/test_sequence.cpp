#include <unity.h>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::sources;

void test_sequence_yields_all_values_then_ends() {
  int pushedValue = 0;
  bool isEnded = false;
  auto iterOverSequence = sequence(1, 15, 2);
  auto pull = iterOverSequence([&pushedValue, &isEnded](Endable<int> v) {
    if (v.hasValue()) {
      pushedValue = v.value();
    } else {
      isEnded = true;
    };
  });
  for (int i = 1; i <= 15; i += 2) {
    TEST_ASSERT_FALSE_MESSAGE(isEnded, "shouldn't be ended until the last value");
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "should get next value in sequence");
  }
  pull();
  TEST_ASSERT_TRUE_MESSAGE(isEnded, "should be done now");
}

void test_sequence_can_be_bound_to_twice_and_yield_twice() {
  int pushedValue1 = 0;
  bool isEnded1 = false;
  int pushedValue2 = 0;
  bool isEnded2 = false;
  auto iterOverSequence = sequence(1, 15, 2);
  auto pull1 = iterOverSequence([&pushedValue1, &isEnded1](Endable<int> v) {
    if (v.hasValue()) {
      pushedValue1 = v.value();
    } else {
      isEnded1 = true;
    }
  });
  auto pull2 = iterOverSequence([&pushedValue2, &isEnded2](Endable<int> v) {
    if (v.hasValue()) {
      pushedValue2 = v.value();
    } else {
      isEnded2 = true;
    }
  });
  for (int i = 1; i <= 15; i += 2) {
    TEST_ASSERT_FALSE_MESSAGE(isEnded1, "first bound listener shouldn't be ended until the last value");
    pull1();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue1, "first bound listener should get next value in vector");
  }
  pull1();
  TEST_ASSERT_TRUE_MESSAGE(isEnded1, "first bound listener should be done now");
  for (int i = 1; i <= 15; i += 2) {
    TEST_ASSERT_FALSE_MESSAGE(isEnded2, "second bound listener shouldn't be ended until the last value");
    pull2();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue2, "second bound listener should get next value in vector");
  }
  pull2();
  TEST_ASSERT_TRUE_MESSAGE(isEnded2, "second bound listener should be done now");
}

void test_sequence_can_go_backwards() {
  int pushedValue = 0;
  bool isEnded = false;
  auto iterOverSequence = sequence(1, -10, -1);
  auto pull = iterOverSequence(
    [&pushedValue, &isEnded](Endable<int> v) {
      if (v.hasValue()) {
        pushedValue = v.value();
      } else {
        isEnded = true;
      }
    }
  );
  for (int i = 1; i >= -10; i -= 1) {
    TEST_ASSERT_FALSE_MESSAGE(isEnded, "shouldn't be ended until the last value");
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "should get next value in sequence");
  }
  pull();
  TEST_ASSERT_TRUE_MESSAGE(isEnded, "should be done now");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_sequence_yields_all_values_then_ends);
  RUN_TEST(test_sequence_can_be_bound_to_twice_and_yield_twice);
  RUN_TEST(test_sequence_can_go_backwards);
  UNITY_END();
}
