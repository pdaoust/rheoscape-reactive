#include <unity.h>
#include <functional>
#include <operators/take.hpp>
#include <sources/constant.hpp>

void test_take_takes() {
  auto source = rheo::constant(5);
  auto taker = rheo::take(source, 8);
  bool didEnd = false;
  int pushedValue = 0;
  int pushedCount = 0;
  auto pull = taker(
    [&pushedValue, &pushedCount](int v) {
      pushedValue = v;
      pushedCount ++;
    },
    [&didEnd]() { didEnd = true; }
  );
  for (int i = 1; i <= 8; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedCount, "should push");
    TEST_ASSERT_EQUAL_MESSAGE(5, pushedValue, "should push correct value");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(8, pushedCount, "should not have pushed anything after end of take");
  TEST_ASSERT_TRUE_MESSAGE(didEnd, "should end after take");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_take_takes);
  UNITY_END();
}