#include <unity.h>
#include <sources/empty.hpp>

void test_empty_never_pushes() {
  int pushedCount = 0;
  auto pull = rheo::empty<int>(
    [&pushedCount](int v) { pushedCount ++; },
    [](){}
  );
  for (int i = 0; i < 3; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "should never push");
  }
}

void test_empty_is_never_done() {
  int endedCount = 0;
  auto pull = rheo::empty<int>(
    [](int v){},
    [&endedCount](){ endedCount ++; }
  );
  TEST_ASSERT_EQUAL_MESSAGE(0, endedCount, "should not end on subscribe");
  for (int i = 0; i < 3; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(0, endedCount, "should not end on pull");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_empty_never_pushes);
  RUN_TEST(test_empty_is_never_done);
  UNITY_END();
}
