#include <unity.h>
#include <functional>
#include <operators/latch.hpp>
#include <operators/unwrap.hpp>
#include <sources/from_iterator.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_latch_latches() {
  std::vector<std::optional<int>> values {
    std::nullopt,
    1,
    std::nullopt,
    std::nullopt,
    2,
    3
  };
  auto some_numbers = unwrap_endable(from_iterator(values.begin(), values.end()));
  auto latcher = latch(some_numbers);
  int push_count = false;
  int pushed_value;
  pull_fn pull = latcher([&push_count, &pushed_value](int v) { push_count ++; pushed_value = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, push_count, "Should not push before first non-empty upstream value is pushed");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count, "Should start pushing after first non-empty upstream value is pushed");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value, "Should push the right value");
  pull();
  TEST_ASSERT_TRUE_MESSAGE(1, "Should not be pushing while empty upstream values are being pushed");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value, "Should push the right value");
  pull();
  TEST_ASSERT_TRUE_MESSAGE(1, "Should not be pushing while empty upstream values are being pushed");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value, "Should push the right value");
  pull();
  TEST_ASSERT_TRUE_MESSAGE(2, "Should push on next non-empty upstream value");
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_value, "Should push the right value");
  pull();
  TEST_ASSERT_TRUE_MESSAGE(3, "Should push on next non-empty upstream value");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_value, "Should push the right value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_latch_latches);
  UNITY_END();
}