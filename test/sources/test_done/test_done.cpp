#include <unity.h>
#include <sources/done.hpp>

void test_done_is_done_on_subscribe() {
  int endedCount = 0;
  int pushedCount = 0;
  auto pull = rheo::done<int>(
    [&pushedCount](int v) { pushedCount ++; },
    [&endedCount](){ endedCount ++; }
  );
  TEST_ASSERT_EQUAL_MESSAGE(1, endedCount, "after subscribing, should've ended immediately");
  TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "should never push");
}

void test_done_is_still_done_on_every_pull() {
  int endedCount = 0;
  int pushedCount = 0;
  auto pull = rheo::done<int>(
    [&pushedCount](int v) { pushedCount ++; },
    [&endedCount](){ endedCount ++; }
  );
  for (int i = 2; i <= 4; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, endedCount, "should end on every pull");
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "should never push");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_done_is_done_on_subscribe);
  RUN_TEST(test_done_is_still_done_on_every_pull);
  UNITY_END();
}
