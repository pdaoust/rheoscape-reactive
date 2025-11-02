#include <unity.h>
#include <functional>
#include <operators/toggle.hpp>
#include <sources/constant.hpp>
#include <types/State.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;


void test_toggle_toggles() {
  auto valueSource = constant(5);
  State<bool> toggler(false);
  auto toggled = toggle(valueSource, toggler.sourceFn());
  int pushedValue = -1;
  int pushedCount = 0;
  auto pull = toggled(
    [&pushedValue, &pushedCount](int v) {
      pushedValue = v;
      pushedCount ++;
    }
  );

  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_FALSE_MESSAGE(pushedValue >= 0, "Shouldn't push value");
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "Shouldn't push");
  }
  toggler.set(true);
  TEST_ASSERT_EQUAL_MESSAGE(5, pushedValue, "Should push value when toggle source changes");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should push");
  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(5, pushedValue, "Pushed value shouldn't change");
    TEST_ASSERT_EQUAL_MESSAGE(i + 2, pushedCount, "Shouldn push");
  }
  toggler.set(false);
  TEST_ASSERT_EQUAL_MESSAGE(6, pushedCount, "Shouldn't push once toggler pushes false");
  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(6, pushedCount, "Shouldn't push anymore");
  }
}

int main (int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_toggle_toggles);
  UNITY_END();
}