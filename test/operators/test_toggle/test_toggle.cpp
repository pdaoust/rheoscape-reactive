#include <unity.h>
#include <functional>
#include <operators/toggle.hpp>
#include <sources/constant.hpp>
#include <states/MemoryState.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;
using namespace rheoscape::states;

void test_toggle_toggles() {
  auto value_source = constant(5);
  MemoryState<bool> toggler(false);
  auto toggled = toggle(value_source, toggler.get_source_fn());
  int pushed_value = -1;
  int pushed_count = 0;
  auto pull = toggled(
    [&pushed_value, &pushed_count](int v) {
      pushed_value = v;
      pushed_count ++;
    }
  );

  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_FALSE_MESSAGE(pushed_value >= 0, "Shouldn't push value");
    TEST_ASSERT_EQUAL_MESSAGE(0, pushed_count, "Shouldn't push");
  }
  toggler.set(true);
  TEST_ASSERT_EQUAL_MESSAGE(5, pushed_value, "Should push value when toggle source changes");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should push");
  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(5, pushed_value, "Pushed value shouldn't change");
    TEST_ASSERT_EQUAL_MESSAGE(i + 2, pushed_count, "Shouldn push");
  }
  toggler.set(false);
  TEST_ASSERT_EQUAL_MESSAGE(6, pushed_count, "Shouldn't push once toggler pushes false");
  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(6, pushed_count, "Shouldn't push anymore");
  }
}

int main (int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_toggle_toggles);
  UNITY_END();
}