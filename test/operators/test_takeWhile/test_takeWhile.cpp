#include <unity.h>
#include <functional>
#include <operators/takeWhile.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_takeWhile_takes() {
  auto source = unwrapEndable(sequence(0, 20, 1));
  auto taker = takeWhile(source, [](auto v) { return v <= 10; });
  bool didEnd = false;
  int pushedValue = 0;
  int pushedCount = 0;
  auto pull = taker(
    [&didEnd, &pushedValue, &pushedCount](auto v) {
      pushedCount ++;
      if (v.hasValue()) {
        pushedValue = v.value();
      } else {
        didEnd = true;
      }
    }
  );
  for (int i = 0; i <= 10; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "should push");
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "should push correct value");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(12, pushedCount, "should push ended value after end of take");
  TEST_ASSERT_TRUE_MESSAGE(didEnd, "should end after take");
}

void test_takeUntil_takes() {
  auto source = unwrapEndable(sequence(0, 20, 1));
  auto taker = takeUntil(source, [](int v) { return v > 10; });
  bool didEnd = false;
  int pushedValue = 0;
  int pushedCount = 0;
  auto pull = taker(
    [&didEnd, &pushedValue, &pushedCount](auto v) {
      pushedCount ++;
      if (v.hasValue()) {
        pushedValue = v.value();
      } else {
        didEnd = true;
      }
    }
  );
  for (int i = 0; i <= 10; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "should push");
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "should push correct value");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(12, pushedCount, "should push ended value after end of take");
  TEST_ASSERT_TRUE_MESSAGE(didEnd, "should end after take");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_takeWhile_takes);
  RUN_TEST(test_takeUntil_takes);
  UNITY_END();
}