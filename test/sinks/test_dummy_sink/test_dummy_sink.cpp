#include <unity.h>
#include <sources/sequence.hpp>
#include <operators/unwrap.hpp>
#include <operators/cache.hpp>
#include <operators/share.hpp>
#include <sinks/dummy_sink.hpp>

using namespace rheoscape;
using namespace rheoscape::sources;
using namespace rheoscape::operators;
using namespace rheoscape::sinks;

void test_dummy_sink_pulls_shared_source_causing_all_sinks_to_receive_value() {
  // Build the pipeline: sequence -> unwrap endable -> cache -> share.
  auto shared_source = sequence(1, 3, 1)
    | unwrap_endable()
    | cache()
    | share();

  // Bind the dummy sink first; it gives us a pull function.
  pull_fn dummy_pull = dummy_sink()(shared_source);

  // Bind an asserting lambda second; we will NOT use its pull function.
  int received_value = 0;
  bool lambda_was_called = false;
  shared_source([&](int v) {
    received_value = v;
    lambda_was_called = true;
  });

  // Pull via the dummy sink only.
  // Because the source is shared, this should push to both sinks.
  dummy_pull();

  TEST_ASSERT_TRUE_MESSAGE(lambda_was_called, "Lambda sink should have received a value from the shared source");
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, received_value, "Lambda sink should have received the first sequence value");

  // Pull again to verify the next value propagates too.
  lambda_was_called = false;
  dummy_pull();

  TEST_ASSERT_TRUE_MESSAGE(lambda_was_called, "Lambda sink should have received a second value");
  TEST_ASSERT_EQUAL_INT_MESSAGE(2, received_value, "Lambda sink should have received the second sequence value");

  // Pull two more times as an integration/smoke test that it works with the cache operator,
  // which is sort of out of scope for this test but is in line with the purpose of `dummy` so it's okay.
  dummy_pull();
  lambda_was_called = false;
  dummy_pull();

  TEST_ASSERT_TRUE_MESSAGE(lambda_was_called, "Lambda sink should have received a second value");
  TEST_ASSERT_EQUAL_INT_MESSAGE(3, received_value, "Lambda sink should have received the second sequence value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_dummy_sink_pulls_shared_source_causing_all_sinks_to_receive_value);
  UNITY_END();
}
