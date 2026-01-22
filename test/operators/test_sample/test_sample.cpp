#include <unity.h>
#include <functional>
#include <operators/sample.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <sources/from_clock.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_sample_samples_on_events() {
  auto clock = from_clock<mock_clock_ulong_millis>();
  State<bool> sampler_event_stream(true);
  auto sampler = sample(sampler_event_stream.get_source_fn(), clock);
  mock_clock_ulong_millis::time_point pushed_value;
  sampler([&pushed_value](mock_clock_ulong_millis::time_point v) { pushed_value = v; });
  // Try to sample the clock with every push to the event stream.
  for (int i = 1; i <= 10; i ++) {
    mock_clock_ulong_millis::tick();
    sampler_event_stream.set(true);
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value.time_since_epoch().count(), "Sample should've pushed sampled value on push event");
  }
}

void test_sample_doesnt_sample_on_events_from_push_stream() {
  State<int> sampled_stream(0);
  State<bool> sampler_event_stream(true);
  auto sampler = sample(sampler_event_stream.get_source_fn(), sampled_stream.get_source_fn());
  int pushed_value = 0;
  sampler([&pushed_value](int v) { pushed_value = v; });
  // We shouldn't get a new value when the sampled stream pushes something.
  sampled_stream.set(1);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(1, pushed_value, "Shouldn't get new value");
  // But we should get it when the event stream pushes something.
  sampler_event_stream.set(true);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value, "Should get new value");
}

void test_sample_handles_reentrant_push_from_downstream() {
  // This test verifies that sample doesn't emit multiple times when
  // the downstream pipeline pushes back to the sampled source.
  //
  // Scenario (like the button pipeline in main.cpp):
  // 1. Event arrives, sample pulls State, emits State value
  // 2. Downstream processes, eventually calls State.set(new_value)
  // 3. State.set() pushes to sample's handler
  // 4. Sample should NOT emit again (event was already consumed)

  State<int> state(0);
  // Use initial_push=false to avoid emission during binding
  State<bool> event_source(false, false);

  int emit_count = 0;
  int last_value = -1;

  // Build a pipeline where sample's output feeds back to the sampled State
  auto sampler = sample(event_source.get_source_fn(false), state.get_source_fn(false));
  sampler([&](int sampled_value) {
    emit_count++;
    last_value = sampled_value;
    // Simulate downstream pipeline that updates the State
    // (like: map(increment) | state.get_setter_sink_fn())
    state.set(sampled_value + 1);
  });

  // No emissions should have happened yet (initial_push=false)
  TEST_ASSERT_EQUAL_MESSAGE(0, emit_count, "No emissions during binding");

  // Trigger an event - should emit exactly once
  event_source.set(true);

  TEST_ASSERT_EQUAL_MESSAGE(1, emit_count,
    "Should emit exactly once, not multiple times from re-entrant State.set()");
  TEST_ASSERT_EQUAL_MESSAGE(0, last_value,
    "Should have sampled the State value at the time of the event");

  // State should now be 1 (from the downstream set)
  TEST_ASSERT_EQUAL_MESSAGE(1, state.get(),
    "State should have been updated by downstream");

  // Another event should sample the new State value
  emit_count = 0;
  event_source.set(true);

  TEST_ASSERT_EQUAL_MESSAGE(1, emit_count, "Second event should emit once");
  TEST_ASSERT_EQUAL_MESSAGE(1, last_value, "Should sample the updated State value");
  TEST_ASSERT_EQUAL_MESSAGE(2, state.get(), "State should be incremented again");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_sample_samples_on_events);
  RUN_TEST(test_sample_doesnt_sample_on_events_from_push_stream);
  RUN_TEST(test_sample_handles_reentrant_push_from_downstream);
  UNITY_END();
}