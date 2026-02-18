#include <unity.h>
#include <types/State.hpp>
#include <operators/dedupe.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;

void test_dedupe_dedupes() {
  rheoscape::State<int> state;
  auto deduped_state = dedupe(state.get_source_fn());
  int value = 0;
  int pushed_count = 0;
  auto pull = deduped_state(
    [&value, &pushed_count](int v) { value = v; pushed_count ++; }
  );

  state.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should push on initial value");
  TEST_ASSERT_EQUAL_MESSAGE(1, value, "Pushed value should be correct");

  state.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, value, "pushed value should still be correct");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should not push again when same value is pushed");

  state.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, value, "pushed value should still be correct");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should not push again when same value is pushed");

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should not push again on pull when value is unchanged");

  state.set(5);
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_count, "Should push on changed value");
  TEST_ASSERT_EQUAL_MESSAGE(5, value, "Pushed value should be different");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_dedupe_dedupes);
  UNITY_END();
}