#include <unity.h>
#include <functional>
#include <Endable.hpp>
#include <operators/take.hpp>
#include <sources/constant.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_take_takes() {
  auto source = constant(5);
  auto taker = take(source, 8);
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
  for (int i = 1; i <= 8; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedCount, "should push");
    TEST_ASSERT_EQUAL_MESSAGE(5, pushedValue, "should push correct value");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(9, pushedCount, "should push ended value after take is finished");
  TEST_ASSERT_TRUE_MESSAGE(didEnd, "should end after take");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_take_takes);
  UNITY_END();
}