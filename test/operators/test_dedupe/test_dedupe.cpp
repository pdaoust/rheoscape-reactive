#include <unity.h>
#include <types/State.hpp>
#include <operators/dedupe.hpp>

using namespace rheo;
using namespace rheo::operators;

void test_dedupe_dedupes() {
  rheo::State<int> state;
  auto dedupedState = dedupe(state.sourceFn());
  int value = 0;
  int pushedCount = 0;
  auto pull = dedupedState(
    [&value, &pushedCount](int v) { value = v; pushedCount ++; }
  );

  state.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should push on initial value");
  TEST_ASSERT_EQUAL_MESSAGE(1, value, "Pushed value should be correct");

  state.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, value, "pushed value should still be correct");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should not push again when same value is pushed");

  state.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, value, "pushed value should still be correct");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should not push again when same value is pushed");

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should not push again on pull when value is unchanged");

  state.set(5);
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedCount, "Should push on changed value");
  TEST_ASSERT_EQUAL_MESSAGE(5, value, "Pushed value should be different");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_dedupe_dedupes);
  UNITY_END();
}