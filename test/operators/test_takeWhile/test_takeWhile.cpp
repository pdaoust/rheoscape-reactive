#include <unity.h>
#include <functional>
#include <operators/takeWhile.hpp>
#include <sources/sequence.hpp>

void test_takeWhile_takes() {
  auto source = rheo::sequence(0, 20, 1);
  auto taker = rheo::takeWhile(source, (rheo::filter_fn<int>)[](int v) { return v <= 10; });
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
  for (int i = 0; i <= 10; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "should push");
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "should push correct value");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedCount, "should not have pushed anything after end of take");
  TEST_ASSERT_TRUE_MESSAGE(didEnd, "should end after take");
}

void test_takeUntil_takes() {
  auto source = rheo::sequence(0, 20, 1);
  auto taker = rheo::takeUntil(source, (rheo::filter_fn<int>)[](int v) { return v > 10; });
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
  for (int i = 0; i <= 10; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "should push");
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "should push correct value");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedCount, "should not have pushed anything after end of take");
  TEST_ASSERT_TRUE_MESSAGE(didEnd, "should end after take");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_takeWhile_takes);
  RUN_TEST(test_takeUntil_takes);
  UNITY_END();
}