#include <unity.h>
#include <sources/empty.hpp>

using namespace rheo::sources;

void test_empty_never_pushes() {
  int pushedCount = 0;
  auto pull = empty<int>(
    [&pushedCount](int v) { pushedCount ++; }
  );
  for (int i = 0; i < 3; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "should never push");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_empty_never_pushes);
  UNITY_END();
}
