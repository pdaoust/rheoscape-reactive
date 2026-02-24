#include <unity.h>
#include <functional>
#include <tuple>
#include <operators/sample.hpp>
#include <types/mock_clock.hpp>
#include <states/MemoryState.hpp>
#include <sources/from_clock.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;
using namespace rheoscape::states;

void test_sample_samples_on_events() {
  auto clock = from_clock<mock_clock_ulong_millis>();
  MemoryState<bool> sampler_event_stream(true);
  auto sampler = sample(sampler_event_stream.get_source_fn(), clock);
  mock_clock_ulong_millis::time_point pushed_value;
  sampler([&pushed_value](std::tuple<bool, mock_clock_ulong_millis::time_point> v) {
    pushed_value = std::get<1>(v);
  });
  // Try to sample the clock with every push to the event stream.
  for (int i = 1; i <= 10; i ++) {
    mock_clock_ulong_millis::tick();
    sampler_event_stream.set(true);
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value.time_since_epoch().count(), "Sample should've pushed sampled value on push event");
  }
}

void test_sample_doesnt_sample_on_events_from_push_stream() {
  MemoryState<int> sampled_stream(0);
  MemoryState<bool> sampler_event_stream(true);
  auto sampler = sample(sampler_event_stream.get_source_fn(), sampled_stream.get_source_fn());
  int pushed_value = 0;
  sampler([&pushed_value](std::tuple<bool, int> v) {
    pushed_value = std::get<1>(v);
  });
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
  // 1. Event arrives, sample pulls MemoryState, emits tuple of (event, MemoryState value)
  // 2. Downstream processes, eventually calls MemoryState.set(new_value)
  // 3. MemoryState.set() pushes to sample's handler
  // 4. Sample should NOT emit again (event was already consumed)

  MemoryState<int> state(0);
  // Use initial_push=false to avoid emission during binding
  MemoryState<bool> event_source(false, false);

  int emit_count = 0;
  int last_value = -1;

  // Build a pipeline where sample's output feeds back to the sampled MemoryState
  auto sampler = sample(event_source.get_source_fn(false), state.get_source_fn(false));
  sampler([&](std::tuple<bool, int> v) {
    int sampled_value = std::get<1>(v);
    emit_count++;
    last_value = sampled_value;
    // Simulate downstream pipeline that updates the MemoryState
    // (like: map([](bool, int v) { return v + 1; }) | state.get_setter_sink_fn())
    state.set(sampled_value + 1);
  });

  // No emissions should have happened yet (initial_push=false)
  TEST_ASSERT_EQUAL_MESSAGE(0, emit_count, "No emissions during binding");

  // Trigger an event - should emit exactly once
  event_source.set(true);

  TEST_ASSERT_EQUAL_MESSAGE(1, emit_count,
    "Should emit exactly once, not multiple times from re-entrant MemoryState.set()");
  TEST_ASSERT_EQUAL_MESSAGE(0, last_value,
    "Should have sampled the MemoryState value at the time of the event");

  // MemoryState should now be 1 (from the downstream set)
  TEST_ASSERT_EQUAL_MESSAGE(1, state.get(),
    "MemoryState should have been updated by downstream");

  // Another event should sample the new MemoryState value
  emit_count = 0;
  event_source.set(true);

  TEST_ASSERT_EQUAL_MESSAGE(1, emit_count, "Second event should emit once");
  TEST_ASSERT_EQUAL_MESSAGE(1, last_value, "Should sample the updated MemoryState value");
  TEST_ASSERT_EQUAL_MESSAGE(2, state.get(), "MemoryState should be incremented again");
}

void test_sample_returns_tuple_with_event_and_sample() {
  MemoryState<int> sampled_stream(42);
  MemoryState<std::string> event_stream("hello");

  auto sampler = sample(event_stream.get_source_fn(), sampled_stream.get_source_fn());

  std::string last_event = "";
  int last_sample = 0;

  sampler([&](std::tuple<std::string, int> v) {
    last_event = std::get<0>(v);
    last_sample = std::get<1>(v);
  });

  event_stream.set("world");
  TEST_ASSERT_TRUE_MESSAGE(last_event == "world", "Should include event value in tuple");
  TEST_ASSERT_EQUAL_MESSAGE(42, last_sample, "Should include sampled value in tuple");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_sample_samples_on_events);
  RUN_TEST(test_sample_doesnt_sample_on_events_from_push_stream);
  RUN_TEST(test_sample_handles_reentrant_push_from_downstream);
  RUN_TEST(test_sample_returns_tuple_with_event_and_sample);
  UNITY_END();
}
