#include <unity.h>
#include <sources/constant.hpp>

void test_constant_produces_constant_value() {
  auto eleven = rheo::constant(11);
  int pushedValue = 0;
  auto pull = eleven([&pushedValue](int v) { pushedValue = v; }, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedValue, "after pulling, should've been pushed a value of 11");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedValue, "should still be 11");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_constant_produces_constant_value);
  UNITY_END();
}
