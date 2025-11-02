#include <unity.h>
#include <sources/done.hpp>

using namespace rheo;
using namespace rheo::sources;

void test_done_is_still_done_on_every_pull() {
  int pushedCount = 0;
  int endedCount = 0;
  auto doneSource = done<int>();
  auto pull = doneSource([&pushedCount, &endedCount](Endable<int> v) {
    pushedCount ++;
    if (!v.hasValue()) {
      endedCount ++;
    }
  });
  for (int i = 0; i <= 4; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "should push once");
    TEST_ASSERT_EQUAL_MESSAGE(1, endedCount, "should be ended on first push");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_done_is_still_done_on_every_pull);
  UNITY_END();
}
