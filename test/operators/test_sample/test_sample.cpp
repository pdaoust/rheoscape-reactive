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

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_sample_samples_on_events);
  RUN_TEST(test_sample_doesnt_sample_on_events_from_push_stream);
  UNITY_END();
}