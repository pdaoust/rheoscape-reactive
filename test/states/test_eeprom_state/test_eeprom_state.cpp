#include <unity.h>
#include <states/EepromState.hpp>
#include <types/mock_clock.hpp>
#include <util/pipes.hpp>
#include <sources/from_clock.hpp>
#include <operators/settle.hpp>

using namespace rheoscape;
using namespace rheoscape::util;
using namespace rheoscape::states;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void setUp() {
  EEPROM.clear();
}

void tearDown() {}

// Each test uses a unique Offset template value
// to get a fresh singleton instance,
// avoiding stale _sinks between tests.

void test_basic_read_write_round_trip() {
  auto [state] = make_eeprom_states<0, int>();

  state.set(42);
  TEST_ASSERT_TRUE(state.has_value());
  TEST_ASSERT_EQUAL(42, state.get());

  auto opt = state.try_get();
  TEST_ASSERT_TRUE(opt.has_value());
  TEST_ASSERT_EQUAL(42, opt.value());
}

void test_uninitialized_eeprom_returns_no_value() {
  // Use a different offset so this is a fresh singleton.
  auto [state] = make_eeprom_states<100, int>();

  TEST_ASSERT_FALSE(state.has_value());
  TEST_ASSERT_FALSE(state.try_get().has_value());
}

void test_garbled_data_rejected() {
  auto [state] = make_eeprom_states<200, int>();

  state.set(12345);
  TEST_ASSERT_TRUE(state.has_value());

  // Corrupt a data byte.
  // SizedHashedWrapper layout: 2 bytes hash, then sizeof(int) bytes data.
  EEPROM[200 + 2] ^= 0xFF;

  TEST_ASSERT_FALSE(state.has_value());
  TEST_ASSERT_FALSE(state.try_get().has_value());
}

void test_zero_checksum_rejected() {
  auto [state] = make_eeprom_states<210, int>();

  state.set(99);
  TEST_ASSERT_TRUE(state.has_value());

  // Zero out the 2-byte hash at the start.
  EEPROM[210] = 0;
  EEPROM[211] = 0;

  TEST_ASSERT_FALSE(state.has_value());
  TEST_ASSERT_FALSE(state.try_get().has_value());
}

void test_consecutive_slots_dont_overlap() {
  auto [s0, s1, s2] = make_eeprom_states<220, int, float, bool>();

  s0.set(111);
  s1.set(2.5f);
  s2.set(true);

  TEST_ASSERT_EQUAL(111, s0.get());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, s1.get());
  TEST_ASSERT_TRUE(s2.get());
}

void test_overwriting_slot_preserves_neighbours() {
  auto [s0, s1, s2] = make_eeprom_states<250, int, float, bool>();

  s0.set(10);
  s1.set(3.14f);
  s2.set(false);

  // Overwrite the middle slot.
  s1.set(6.28f);

  TEST_ASSERT_EQUAL(10, s0.get());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.28f, s1.get());
  TEST_ASSERT_FALSE(s2.get());
}

void test_push_on_set() {
  auto [state] = make_eeprom_states<280, int>();

  int pushed_value = -1;
  int push_count = 0;
  state.add_sink(
    [&pushed_value, &push_count](int v) {
      pushed_value = v;
      push_count++;
    },
    // No initial push; EEPROM is empty.
    false
  );

  state.set(77);
  TEST_ASSERT_EQUAL(1, push_count);
  TEST_ASSERT_EQUAL(77, pushed_value);

  state.set(88);
  TEST_ASSERT_EQUAL(2, push_count);
  TEST_ASSERT_EQUAL(88, pushed_value);
}

void test_initial_push_on_add_sink() {
  auto [state] = make_eeprom_states<290, int>();

  state.set(55);

  int pushed_value = -1;
  int push_count = 0;
  state.add_sink(
    [&pushed_value, &push_count](int v) {
      pushed_value = v;
      push_count++;
    },
    // initial_push = true (default).
    true
  );

  TEST_ASSERT_EQUAL(1, push_count);
  TEST_ASSERT_EQUAL(55, pushed_value);
}

// -- Mirrored EEPROM pattern tests --
// These use make_memory_mirrored_eeprom_pipes,
// which sets up push-driven pipelines:
//   EEPROM -> memory (initial populate, push on bind)
//   memory -> pipe -> EEPROM (buffered write-back)
//
// The settle pipeline is driven by pushes from memory_state.set().
// Each set() triggers combine to pull the clock for a timestamp,
// so no external pull loop is needed.

using MockDuration = mock_clock_ulong_millis::duration;

void test_buffered_value_doesnt_write_immediately() {
  mock_clock_ulong_millis::set_time(0);

  auto [mem_state] = make_memory_mirrored_eeprom_pipes<300>(
    typed_pipe<int>(settle(from_clock<mock_clock_ulong_millis>(), MockDuration(50)))
  );
  auto& eeprom_state = states::detail::EepromState<int, 300>::get_instance();

  // Set a value on the memory state.
  // Settle sees it at t=0 but hasn't waited long enough.
  mem_state.set(42);

  // EEPROM should not have the value yet.
  TEST_ASSERT_FALSE(eeprom_state.has_value());
}

void test_buffered_value_writes_after_settling() {
  mock_clock_ulong_millis::set_time(0);

  auto [mem_state] = make_memory_mirrored_eeprom_pipes<310>(
    typed_pipe<int>(settle(from_clock<mock_clock_ulong_millis>(), MockDuration(50)))
  );
  auto& eeprom_state = states::detail::EepromState<int, 310>::get_instance();

  // First push records the value and timestamp at t=0.
  mem_state.set(42);
  TEST_ASSERT_FALSE(eeprom_state.has_value());

  // Advance past the settling period.
  mock_clock_ulong_millis::set_time(51);
  // Push again to trigger settle to re-check.
  // Same value, so settle doesn't reset the timer.
  mem_state.set(42);

  TEST_ASSERT_TRUE(eeprom_state.has_value());
  TEST_ASSERT_EQUAL(42, eeprom_state.get());
}

void test_buffered_rapid_changes_write_final_value() {
  mock_clock_ulong_millis::set_time(0);

  auto [mem_state] = make_memory_mirrored_eeprom_pipes<320>(
    typed_pipe<int>(settle(from_clock<mock_clock_ulong_millis>(), MockDuration(50)))
  );
  auto& eeprom_state = states::detail::EepromState<int, 320>::get_instance();

  // Rapid changes; each one resets settle's timer.
  mem_state.set(1);

  mock_clock_ulong_millis::set_time(20);
  mem_state.set(2);

  mock_clock_ulong_millis::set_time(40);
  mem_state.set(3);

  // Not enough time since last change (t=40).
  TEST_ASSERT_FALSE(eeprom_state.has_value());

  // Advance past settling period from last change.
  mock_clock_ulong_millis::set_time(91);
  // Same value push triggers settle to check and emit.
  mem_state.set(3);

  TEST_ASSERT_TRUE(eeprom_state.has_value());
  TEST_ASSERT_EQUAL(3, eeprom_state.get());
}

void test_buffered_revert_before_settling_preserves_original() {
  mock_clock_ulong_millis::set_time(0);

  // Pre-populate EEPROM before setting up buffered pipelines.
  states::detail::EepromState<int, 330>::get_instance().set(100);

  auto [mem_state] = make_memory_mirrored_eeprom_pipes<330>(
    typed_pipe<int>(settle(from_clock<mock_clock_ulong_millis>(), MockDuration(50)))
  );
  auto& eeprom_state = states::detail::EepromState<int, 330>::get_instance();

  // Initial EEPROM value (100) was pushed to mem_state during pipeline binding.
  TEST_ASSERT_EQUAL(100, eeprom_state.get());

  // Change the in-memory value.
  mock_clock_ulong_millis::set_time(10);
  mem_state.set(999);

  // Revert before settling completes.
  mock_clock_ulong_millis::set_time(30);
  mem_state.set(100);

  // Advance past settling period from the revert.
  mock_clock_ulong_millis::set_time(81);
  mem_state.set(100);

  // EEPROM should still have the original value.
  TEST_ASSERT_EQUAL(100, eeprom_state.get());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_basic_read_write_round_trip);
  RUN_TEST(test_uninitialized_eeprom_returns_no_value);
  RUN_TEST(test_garbled_data_rejected);
  RUN_TEST(test_zero_checksum_rejected);
  RUN_TEST(test_consecutive_slots_dont_overlap);
  RUN_TEST(test_overwriting_slot_preserves_neighbours);
  RUN_TEST(test_push_on_set);
  RUN_TEST(test_initial_push_on_add_sink);
  RUN_TEST(test_buffered_value_doesnt_write_immediately);
  RUN_TEST(test_buffered_value_writes_after_settling);
  RUN_TEST(test_buffered_rapid_changes_write_final_value);
  RUN_TEST(test_buffered_revert_before_settling_preserves_original);
  UNITY_END();
}
