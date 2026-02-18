#include <unity.h>
#include <sources/empty.hpp>

using namespace rheoscape::sources;

void test_empty_never_pushes() {
  int pushed_count = 0;
  auto pull = empty<int>(
    [&pushed_count](int v) { pushed_count ++; }
  );
  for (int i = 0; i < 3; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(0, pushed_count, "should never push");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_empty_never_pushes);
  UNITY_END();
}
