#include <unity.h>
#include <functional>
#include <operators/latch.hpp>
#include <operators/unwrap.hpp>
#include <sources/fromIterator.hpp>

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
  auto someNumbers = unwrapEndable(fromIterator(values.begin(), values.end()));
  auto latcher = latch(someNumbers);
  int pushCount = false;
  int pushedValue;
  pull_fn pull = latcher([&pushCount, &pushedValue](int v) { pushCount ++; pushedValue = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, pushCount, "Should not push before first non-empty upstream value is pushed");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushCount, "Should start pushing after first non-empty upstream value is pushed");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "Should push the right value");
  pull();
  TEST_ASSERT_TRUE_MESSAGE(1, "Should not be pushing while empty upstream values are being pushed");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "Should push the right value");
  pull();
  TEST_ASSERT_TRUE_MESSAGE(1, "Should not be pushing while empty upstream values are being pushed");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "Should push the right value");
  pull();
  TEST_ASSERT_TRUE_MESSAGE(2, "Should push on next non-empty upstream value");
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedValue, "Should push the right value");
  pull();
  TEST_ASSERT_TRUE_MESSAGE(3, "Should push on next non-empty upstream value");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedValue, "Should push the right value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_latch_latches);
  UNITY_END();
}