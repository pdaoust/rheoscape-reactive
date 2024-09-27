#include <unity.h>
#include <operators/debounce.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <sources/fromClock.hpp>

void test_debounce_debounces_to_new_state() {
  rheo::mock_clock_ulong_millis::setTime(0);
  auto mockClock = rheo::fromClock<rheo::mock_clock_ulong_millis>();
  rheo::State<bool> button(false);
  auto debouncedButton = rheo::debounce(button.sourceFn(), mockClock, rheo::mock_clock_ulong_millis::duration(50));
  bool buttonState = false;
  int pushedCount = 0;
  rheo::pull_fn pull = debouncedButton(
    [&buttonState, &pushedCount](bool v) { buttonState = v; pushedCount ++; },
    [](){}
  );

  // Get the clock going.
  for (int i = 0; i < 50; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should be false when not pressed");
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "Should not have pushed anything before the initial state is settled");
  }

  for (int i = 0; i < 50; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should be false when not pressed");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should have pushed a false value after the initial state was settled");
  }

  // Now set the button high, but fluctuate for 1ms less than the debounce period
  // before settling to high.
  for (int i = 1; i <= 50; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    // Noisy button!
    button.set((bool)i % 2);
    // Pull not needed because the State just pushed on change.
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should still be false while waiting for bouncy period to settle");
    TEST_ASSERT_EQUAL_MESSAGE(i + 50, pushedCount, "Should have pushed that false value because upstream pushed");
  }
  
  // At the end, after one more tick
  // (to push it past the debounce period)
  // it should have settled to true.
  rheo::mock_clock_ulong_millis::tick();
  // Because it ends on 50, which % 2 is false,
  // we want to make sure it's true.
  button.set(true);
  TEST_ASSERT_TRUE_MESSAGE(buttonState, "Should now be true after debounce period has settled");
  TEST_ASSERT_EQUAL_MESSAGE(101, pushedCount, "Should have pushed one more time");
}

void test_debounce_debounces_to_old_state() {
  rheo::mock_clock_ulong_millis::setTime(0);
  auto mockClock = rheo::fromClock<rheo::mock_clock_ulong_millis>();
  rheo::State<bool> button(false);
  auto debouncedButton = rheo::debounce(button.sourceFn(), mockClock, rheo::mock_clock_ulong_millis::duration(50));
  bool buttonState = false;
  int pushedCount = 0;
  rheo::pull_fn pull = debouncedButton(
    [&buttonState, &pushedCount](bool v) { buttonState = v; pushedCount ++; },
    [](){}
  );

  // Get the clock going.
  for (int i = 0; i < 50; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should be false when not pressed");
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "Should not have pushed anything before the initial state is settled");
  }

  for (int i = 0; i < 50; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should be false when not pressed");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should have pushed a false value after the initial state was settled");
  }

  // Now set the button high, but fluctuate for 1ms less than the debounce period
  // before settling to low.
  for (int i = 1; i <= 50; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    // Noisy button!
    button.set((bool)i % 2);
    // Pull not needed because the State just pushed on change.
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should still be false while waiting for bouncy period to settle");
    TEST_ASSERT_EQUAL_MESSAGE(i + 50, pushedCount, "Should have pushed that false value because upstream pushed");
  }
  
  // At the end, after one more tick
  // (to push it past the debounce period)
  // it should have settled to the old false state
  // because the ending value of i in the loop
  // is 50 % 2 = 0.
  rheo::mock_clock_ulong_millis::tick();
  pull();
  TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should have reverted to old state after debounce period has settled");
  TEST_ASSERT_EQUAL_MESSAGE(101, pushedCount, "Should have pushed one more time");
}

void test_debounce_debounces_tri_state() {
  rheo::mock_clock_ulong_millis::setTime(0);
  auto mockClock = rheo::fromClock<rheo::mock_clock_ulong_millis>();
  rheo::State<int> button(0);
  auto debouncedButton = rheo::debounce(button.sourceFn(), mockClock, rheo::mock_clock_ulong_millis::duration(50));
  int buttonState = -1;
  int pushedCount = 0;
  rheo::pull_fn pull = debouncedButton(
    [&buttonState, &pushedCount](int v) { buttonState = v; pushedCount ++; },
    [](){}
  );

  // Get the clock going.
  for (int i = 0; i < 50; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(-1, buttonState, "Should not have a state yet");
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "Should not have pushed anything before the initial state is settled");
  }

  for (int i = 0; i < 50; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(0, buttonState, "Should be 0 when initial state is settled");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should have pushed a false value after the initial state was settled");
  }

  // Now set the button to 1, but fluctuate for 1ms less than the debounce period
  // before settling to 2.
  for (int i = 1; i < 50; i ++) {
    rheo::mock_clock_ulong_millis::tick();
    // Noisy button!
    button.set(i % 3);
    // Pull not needed because the State just pushed on change.
    TEST_ASSERT_EQUAL_MESSAGE(0, buttonState, "Should still be false while waiting for bouncy period to settle");
    TEST_ASSERT_EQUAL_MESSAGE(i + 50, pushedCount, "Should have pushed that false value because upstream pushed");
  }
  
  // At the end, after one more tick
  // (to push it past the debounce period)
  // it should not have settled on the new state,
  // but reverted back to the initial state.
  rheo::mock_clock_ulong_millis::tick();
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, buttonState, "Should now be true after debounce period has settled");
  TEST_ASSERT_EQUAL_MESSAGE(101, pushedCount, "Should have pushed one more time");

  // But if we keep going past the debounce period,
  // we find that it's settled to the state that it ended on!
  rheo::mock_clock_ulong_millis::tick(50);
  TEST_ASSERT_EQUAL_MESSAGE(2, buttonState, "Should now be true after debounce period has settled");
  TEST_ASSERT_EQUAL_MESSAGE(102, pushedCount, "Should have pushed one more time");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_debounce_debounces_to_new_state);
  UNITY_END();
}