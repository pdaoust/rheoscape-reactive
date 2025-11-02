#include <unity.h>
#include <functional>
#include <operators/sample.hpp>
#include <types/mock_clock.hpp>
#include <types/State.hpp>
#include <sources/fromClock.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_sample_samples_on_events() {
  auto clock = fromClock<mock_clock_ulong_millis>();
  State<bool> samplerEventStream(true);
  auto sampler = sample(samplerEventStream.sourceFn(), clock);
  mock_clock_ulong_millis::time_point pushedValue;
  sampler([&pushedValue](mock_clock_ulong_millis::time_point v) { pushedValue = v; });
  // Try to sample the clock with every push to the event stream.
  for (int i = 1; i <= 10; i ++) {
    mock_clock_ulong_millis::tick();
    samplerEventStream.set(true);
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue.time_since_epoch().count(), "Sample should've pushed sampled value on push event");
  }
}

void test_sample_doesnt_sample_on_events_from_push_stream() {
  State<int> sampledStream(0);
  State<bool> samplerEventStream(true);
  auto sampler = sample(samplerEventStream.sourceFn(), sampledStream.sourceFn());
  int pushedValue = 0;
  sampler([&pushedValue](int v) { pushedValue = v; });
  // We shouldn't get a new value when the sampled stream pushes something.
  sampledStream.set(1);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(1, pushedValue, "Shouldn't get new value");
  // But we should get it when the event stream pushes something.
  samplerEventStream.set(true);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "Should get new value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_sample_samples_on_events);
  RUN_TEST(test_sample_doesnt_sample_on_events_from_push_stream);
  UNITY_END();
}