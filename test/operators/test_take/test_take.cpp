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
  bool did_end = false;
  int pushed_value = 0;
  int pushed_count = 0;
  auto pull = taker(
    [&did_end, &pushed_value, &pushed_count](auto v) {
      pushed_count ++;
      if (v.has_value()) {
        pushed_value = v.value();
      } else {
        did_end = true;
      }
    }
  );
  for (int i = 1; i <= 8; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_count, "should push");
    TEST_ASSERT_EQUAL_MESSAGE(5, pushed_value, "should push correct value");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(9, pushed_count, "should push ended value after take is finished");
  TEST_ASSERT_TRUE_MESSAGE(did_end, "should end after take");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_take_takes);
  UNITY_END();
}