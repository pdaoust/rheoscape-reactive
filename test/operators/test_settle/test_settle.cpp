#include <unity.h>
#include <operators/settle.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <sources/from_clock.hpp>

using namespace rheo;
using namespace operators;
using namespace sources;

void test_settle_waits_for_stable_value() {
  mock_clock_ulong_millis::set_time(0);
  auto mock_clock = from_clock<mock_clock_ulong_millis>();
  State<int> input(10);
  auto settled = settle(input.get_source_fn(), mock_clock, mock_clock_ulong_millis::duration(50));
  int pushed_value = -1;
  int push_count = 0;
  pull_fn pull = settled(
    [&pushed_value, &push_count](int v) { pushed_value = v; push_count++; }
  );

  // Value hasn't settled yet — no output expected.
  // First pull at t=1 records the initial timestamp.
  // We need interval (50) more ms after that first observation, i.e. t=51.
  for (int i = 0; i < 50; i++) {
    mock_clock_ulong_millis::tick();
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(0, push_count, "should not push before settling period elapses");
  }

  // One more tick puts us past the interval from the first observation.
  mock_clock_ulong_millis::tick();
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count, "should push after settling period elapses");
  TEST_ASSERT_EQUAL_MESSAGE(10, pushed_value, "should push the settled value");
}

void test_settle_resets_on_value_change() {
  mock_clock_ulong_millis::set_time(0);
  auto mock_clock = from_clock<mock_clock_ulong_millis>();
  State<int> input(10);
  auto settled = settle(input.get_source_fn(), mock_clock, mock_clock_ulong_millis::duration(50));
  int pushed_value = -1;
  int push_count = 0;
  pull_fn pull = settled(
    [&pushed_value, &push_count](int v) { pushed_value = v; push_count++; }
  );

  // Advance 40ms with value 10.
  for (int i = 0; i < 40; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  TEST_ASSERT_EQUAL_MESSAGE(0, push_count, "should not push before settling period");

  // Change the value at t=40. This should reset the settling timer.
  input.set(20);

  // Advance another 10ms (t=50). Only 10ms since the change — not enough.
  for (int i = 0; i < 10; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  TEST_ASSERT_EQUAL_MESSAGE(0, push_count,
    "should not push — value changed at t=40, only 10ms have passed since");

  // At t=90 (50ms after the change at t=40), the new value should have settled.
  for (int i = 0; i < 40; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count, "should push after new value settles");
  TEST_ASSERT_EQUAL_MESSAGE(20, pushed_value, "should push the new settled value");
}

void test_settle_discards_intermediate_values() {
  // This is the key difference from debounce:
  // if the value changes during the settling period, settle resets the timer
  // and only emits the final stable value.
  mock_clock_ulong_millis::set_time(0);
  auto mock_clock = from_clock<mock_clock_ulong_millis>();
  State<int> input(1);
  auto settled = settle(input.get_source_fn(), mock_clock, mock_clock_ulong_millis::duration(50));
  int pushed_value = -1;
  int push_count = 0;
  pull_fn pull = settled(
    [&pushed_value, &push_count](int v) { pushed_value = v; push_count++; }
  );

  // value=1 at t=0. Advance 20ms.
  for (int i = 0; i < 20; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }

  // Change to value=2 at t=20. Advance 20ms.
  input.set(2);
  for (int i = 0; i < 20; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }

  // Change to value=3 at t=40. Advance 20ms.
  input.set(3);
  for (int i = 0; i < 20; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }

  TEST_ASSERT_EQUAL_MESSAGE(0, push_count,
    "should not have emitted anything — no value has been stable for 50ms");

  // Now hold steady at value=3 for 30 more ms (total 50ms since t=40).
  for (int i = 0; i < 30; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count, "should emit after value=3 has been stable for 50ms");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_value, "should emit the final settled value, not intermediates");
}

void test_settle_continues_emitting_stable_value_on_pull() {
  mock_clock_ulong_millis::set_time(0);
  auto mock_clock = from_clock<mock_clock_ulong_millis>();
  State<int> input(42);
  auto settled = settle(input.get_source_fn(), mock_clock, mock_clock_ulong_millis::duration(50));
  int pushed_value = -1;
  int push_count = 0;
  pull_fn pull = settled(
    [&pushed_value, &push_count](int v) { pushed_value = v; push_count++; }
  );

  // Let the value settle (need interval+1 ticks from first observation).
  for (int i = 0; i < 51; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count, "first push after settling");

  // Subsequent pulls should continue to emit the settled value.
  mock_clock_ulong_millis::tick();
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, push_count, "should push on each pull after settling");
  TEST_ASSERT_EQUAL_MESSAGE(42, pushed_value, "should still emit 42");
}

void test_settle_does_not_push_during_bounce_from_push_source() {
  // When the upstream pushes without a pull (e.g. State::set triggers push),
  // settle should NOT emit values that are still bouncing.
  mock_clock_ulong_millis::set_time(0);
  auto mock_clock = from_clock<mock_clock_ulong_millis>();
  State<int> input(0);
  auto settled = settle(input.get_source_fn(), mock_clock, mock_clock_ulong_millis::duration(50));
  int pushed_value = -1;
  int push_count = 0;
  pull_fn pull = settled(
    [&pushed_value, &push_count](int v) { pushed_value = v; push_count++; }
  );

  // Settle the initial value.
  for (int i = 0; i < 51; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count, "initial settle");

  // Now rapidly change via push (State::set) without pulling.
  input.set(1);
  input.set(2);
  input.set(3);
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count,
    "should not emit during push-driven bounces without a pull");
}

void test_settle_incomplete_turn_no_output() {
  // Value changes but reverts before the settling period elapses.
  // No new value should be emitted.
  mock_clock_ulong_millis::set_time(0);
  auto mock_clock = from_clock<mock_clock_ulong_millis>();
  State<int> input(0);
  auto settled = settle(input.get_source_fn(), mock_clock, mock_clock_ulong_millis::duration(50));
  int pushed_value = -1;
  int push_count = 0;
  pull_fn pull = settled(
    [&pushed_value, &push_count](int v) { pushed_value = v; push_count++; }
  );

  // Settle initial value of 0.
  for (int i = 0; i < 51; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count, "initial settle");
  TEST_ASSERT_EQUAL_MESSAGE(0, pushed_value, "initial value is 0");

  // Change to 5 for 30ms, then revert back to 0.
  input.set(5);
  for (int i = 0; i < 30; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  input.set(0);

  // The reverted value (0) is the same as the already-settled value,
  // so pulling should just keep emitting 0 — value 5 should never appear.
  for (int i = 0; i < 60; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  TEST_ASSERT_EQUAL_MESSAGE(0, pushed_value, "should never have emitted the transient value 5");
}

void test_settle_pipe_factory() {
  mock_clock_ulong_millis::set_time(0);
  auto mock_clock = from_clock<mock_clock_ulong_millis>();
  State<int> input(99);
  auto settled = input.get_source_fn()
    | settle(mock_clock, mock_clock_ulong_millis::duration(30));
  int pushed_value = -1;
  int push_count = 0;
  pull_fn pull = settled(
    [&pushed_value, &push_count](int v) { pushed_value = v; push_count++; }
  );

  for (int i = 0; i < 31; i++) {
    mock_clock_ulong_millis::tick();
    pull();
  }
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count, "should settle via pipe factory");
  TEST_ASSERT_EQUAL_MESSAGE(99, pushed_value, "should emit the settled value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_settle_waits_for_stable_value);
  RUN_TEST(test_settle_resets_on_value_change);
  RUN_TEST(test_settle_discards_intermediate_values);
  RUN_TEST(test_settle_continues_emitting_stable_value_on_pull);
  RUN_TEST(test_settle_does_not_push_during_bounce_from_push_source);
  RUN_TEST(test_settle_incomplete_turn_no_output);
  RUN_TEST(test_settle_pipe_factory);
  UNITY_END();
}
