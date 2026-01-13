#include <unity.h>
#include <functional>
#include <operators/take_while.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_take_while_takes() {
  auto source = unwrap_endable(sequence(0, 20, 1));
  auto taker = take_while(source, [](auto v) { return v <= 10; });
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
  for (int i = 0; i <= 10; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushed_count, "should push");
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value, "should push correct value");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(12, pushed_count, "should push ended value after end of take");
  TEST_ASSERT_TRUE_MESSAGE(did_end, "should end after take");
}

void test_take_until_takes() {
  auto source = unwrap_endable(sequence(0, 20, 1));
  auto taker = take_until(source, [](int v) { return v > 10; });
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
  for (int i = 0; i <= 10; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushed_count, "should push");
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value, "should push correct value");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(12, pushed_count, "should push ended value after end of take");
  TEST_ASSERT_TRUE_MESSAGE(did_end, "should end after take");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_take_while_takes);
  RUN_TEST(test_take_until_takes);
  UNITY_END();
}