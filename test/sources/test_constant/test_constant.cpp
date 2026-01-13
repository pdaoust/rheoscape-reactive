#include <unity.h>
#include <sources/constant.hpp>

using namespace rheo::sources;

void test_constant_produces_constant_value() {
  auto eleven = constant(11);
  int pushed_value = 0;
  auto pull = eleven([&pushed_value](int v) { pushed_value = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushed_value, "after pulling, should've been pushed a value of 11");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushed_value, "should still be 11");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_constant_produces_constant_value);
  UNITY_END();
}
