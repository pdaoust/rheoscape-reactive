#include <unity.h>
#include <sources/done.hpp>

using namespace rheoscape;
using namespace rheoscape::sources;

void test_done_is_still_done_on_every_pull() {
  int pushed_count = 0;
  int ended_count = 0;
  auto done_source = done<int>();
  auto pull = done_source([&pushed_count, &ended_count](Endable<int> v) {
    pushed_count ++;
    if (!v.has_value()) {
      ended_count ++;
    }
  });
  for (int i = 0; i <= 4; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "should push once");
    TEST_ASSERT_EQUAL_MESSAGE(1, ended_count, "should be ended on first push");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_done_is_still_done_on_every_pull);
  UNITY_END();
}
