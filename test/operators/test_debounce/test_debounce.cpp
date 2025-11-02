#include <unity.h>
#include <operators/debounce.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <sources/fromClock.hpp>

using namespace rheo;
using namespace operators;
using namespace sources;

void test_debounce_debounces_to_new_state() {
  mock_clock_ulong_millis::setTime(0);
  auto mockClock = fromClock<mock_clock_ulong_millis>();
  State<bool> button(false);
  auto debouncedButton = debounce(button.sourceFn(), mockClock, mock_clock_ulong_millis::duration(50));
  bool buttonState = false;
  int pushedCount = 0;
  pull_fn pull = debouncedButton(
    [&buttonState, &pushedCount](bool v) { buttonState = v; pushedCount ++; }
  );

  // Get the clock going.
  for (int i = 0; i < 50; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should be false when not pressed");
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "Should not have pushed anything before the initial state is settled");
  }

  for (int i = 0; i < 50; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should be false when not pressed");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should push a false value when pulled after initial settling");
  }

  // Now set the button high, but fluctuate for 1ms less than the debounce period
  // before settling to high.
  for (int i = 1; i <= 49; i ++) {
    mock_clock_ulong_millis::tick();
    // Noisy button!
    button.set((bool)(i % 2));
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should still be false while waiting for bouncy period to settle");
    TEST_ASSERT_EQUAL_MESSAGE(i + 49, pushedCount, "Should not have pushed anything just because upstream pushed");
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 50, pushedCount, "Should only push a new value for unsettled button when pulled");
  }
  
  // At the end, after two more ticks
  // (to push it past the debounce period)
  // it should have settled to true.
  mock_clock_ulong_millis::tick();
  mock_clock_ulong_millis::tick();
  pull();
  TEST_ASSERT_TRUE_MESSAGE(buttonState, "Should now be true after debounce period has settled");
}

void test_debounce_debounces_to_old_state() {
  mock_clock_ulong_millis::setTime(0);
  auto mockClock = fromClock<mock_clock_ulong_millis>();
  State<bool> button(false);
  auto debouncedButton = debounce(button.sourceFn(), mockClock, mock_clock_ulong_millis::duration(50));
  bool buttonState = false;
  int pushedCount = 0;
  pull_fn pull = debouncedButton(
    [&buttonState, &pushedCount](bool v) { buttonState = v; pushedCount ++; }
  );

  // Get the clock going.
  for (int i = 0; i < 50; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should be false when not pressed");
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "Should not have pushed anything before the initial state is settled");
  }

  for (int i = 0; i < 50; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should be false when not pressed");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should push a false value when pulled after initial settling");
  }

  // Now set the button high, but fluctuate for 2ms less than the debounce period
  // before settling to low.
  for (int i = 1; i <= 48; i ++) {
    mock_clock_ulong_millis::tick();
    // Noisy button!
    button.set((bool)(i % 2));
    // Pull not needed because the State just pushed on change.
    TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should still be false while waiting for bouncy period to settle");
    TEST_ASSERT_EQUAL_MESSAGE(i + 49, pushedCount, "Should not have pushed anything just because upstream pushed");
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i + 50, pushedCount, "Should only push a new value for unsettled button when pulled");
  }
  
  // At the end, after three more tick
  // (to push it past the debounce period)
  // it should have settled to the old false state
  // because the ending value of i in the loop
  // is 50 % 2 = 0.
  mock_clock_ulong_millis::tick();
  mock_clock_ulong_millis::tick();
  mock_clock_ulong_millis::tick();
  pull();
  TEST_ASSERT_FALSE_MESSAGE(buttonState, "Should have reverted to old state after debounce period has settled");
}

void test_debounce_debounces_tri_state() {
  mock_clock_ulong_millis::setTime(0);
  auto mockClock = fromClock<mock_clock_ulong_millis>();
  State<int> button(0);
  auto debouncedButton = debounce(button.sourceFn(), mockClock, mock_clock_ulong_millis::duration(50));
  int buttonState = -1;
  int pushedCount = 0;
  pull_fn pull = debouncedButton(
    [&buttonState, &pushedCount](int v) { buttonState = v; pushedCount ++; }
  );

  // Get the clock going.
  for (int i = 0; i < 50; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(-1, buttonState, "Should not have a state yet");
    TEST_ASSERT_EQUAL_MESSAGE(0, pushedCount, "Should not have pushed anything before the initial state is settled");
  }

  for (int i = 0; i < 50; i ++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(0, buttonState, "Should be 0 when initial state is settled");
    TEST_ASSERT_EQUAL_MESSAGE(i + 1, pushedCount, "Should have pushed a false value after the initial state was settled");
  }

  // Now set the button to 1, but fluctuate for 4ms less than the debounce period
  // before settling to 2.
  for (int i = 1; i <= 47; i ++) {
    mock_clock_ulong_millis::tick();
    // Noisy button!
    button.set(i % 3);
    // Pull not needed because the State just pushed on change.
    TEST_ASSERT_EQUAL_MESSAGE(0, buttonState, "Should still be false while waiting for bouncy period to settle");
  }
  
  // At the end, after four more ticks
  // (to push it past the debounce period)
  // it should not have settled on the new state,
  // but reverted back to the initial state.
  mock_clock_ulong_millis::tick();
  mock_clock_ulong_millis::tick();
  mock_clock_ulong_millis::tick();
  mock_clock_ulong_millis::tick();
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, buttonState, "Should revert to 0 after debounce period has settled");

  // But if we keep going past the debounce period,
  // we find that it's settled to the state that it ended on!
  mock_clock_ulong_millis::tick(50);
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, buttonState, "Should now be 2 after debounce period has settled");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_debounce_debounces_to_new_state);
  RUN_TEST(test_debounce_debounces_to_old_state);
  RUN_TEST(test_debounce_debounces_tri_state);
  UNITY_END();
}