#include <unity.h>
#include <functional>
#include <operators/lift.hpp>
#include <operators/map.hpp>
#include <operators/unwrap.hpp>
#include <types/State.hpp>
#include <sources/fromIterator.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_liftToOptional_can_push() {
  auto doublingPipe = map([](int v) { return v * 2; });
  auto lifted = liftToOptional(doublingPipe);
  State<std::optional<int>> state;
  auto doubledOptionalNumbers = lifted(state.sourceFn());

  std::optional<int> lastPushedValue = std::nullopt;
  doubledOptionalNumbers(
    [&lastPushedValue](std::optional<int> v) { lastPushedValue = v; }
  );

  state.set(3);
  TEST_ASSERT_TRUE_MESSAGE(lastPushedValue.has_value(), "Lowerable should get pushed");
  TEST_ASSERT_EQUAL_MESSAGE(6, lastPushedValue.value(), "Value should get transformed by inner pipe");
  state.set(std::nullopt);
  TEST_ASSERT_FALSE_MESSAGE(lastPushedValue.has_value(), "Non-lowerable value should get pushed too");
}

void test_liftToOptional_can_pull() {
  // Try lifting a mapping pipe that knows nothing about optionals
  // to a mapping pipe that does.
  auto doublingPipe = map([](int v) { return v * 2; });
  auto lifted = liftToOptional(doublingPipe);
  std::vector<std::optional<int>> optionalNumbers {
    3,
    std::nullopt,
    1,
    5,
    std::nullopt
  };
  auto optionalNumbersSource = unwrapEndable(fromIterator(optionalNumbers.begin(), optionalNumbers.end()));
  auto doubledOptionalNumbers = lifted(optionalNumbersSource);

  int pushedCount = 0;
  std::optional<int> lastPushedValue = 0;
  pull_fn pull = doubledOptionalNumbers(
    [&pushedCount, &lastPushedValue](std::optional<int> v) {
      pushedCount ++;
      lastPushedValue = v;
    }
  );

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should push on non-empty pulls only");
  TEST_ASSERT_TRUE_MESSAGE(lastPushedValue.has_value(), "Should have pushed value for non-empty");
  TEST_ASSERT_EQUAL_MESSAGE(6, lastPushedValue.value(), "Should have doubled the non-empty value");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedCount, "Should not push on empty pulls");
  TEST_ASSERT_FALSE_MESSAGE(lastPushedValue.has_value(), "Should have pushed empty value for empty");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedCount, "Should push on non-empty pulls only");
  TEST_ASSERT_TRUE_MESSAGE(lastPushedValue.has_value(), "Should have pushed value for non-empty");
  TEST_ASSERT_EQUAL_MESSAGE(2, lastPushedValue.value(), "Should have doubled the non-empty value");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(4, pushedCount, "Should push on non-empty pulls only");
  TEST_ASSERT_TRUE_MESSAGE(lastPushedValue.has_value(), "Should have pushed value for non-empty");
  TEST_ASSERT_EQUAL_MESSAGE(10, lastPushedValue.value(), "Should have doubled the non-empty value");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(5, pushedCount, "Should not push on empty pulls");
TEST_ASSERT_FALSE_MESSAGE(lastPushedValue.has_value(), "Should have pushed empty value for empty");
}

void test_liftToTaggedValue_lifts() {
  auto doublingPipe = map([](int v) { return v * 2; });
  auto lifted = liftToTaggedValue<int>(doublingPipe);
  std::vector<TaggedValue<int, int>> taggedNumbers {
    TaggedValue { 1, -1 },
    TaggedValue { 2, -2 },
    TaggedValue { 3, -3 },
  };
  auto taggedNumbersSource = unwrapEndable(fromIterator(taggedNumbers.begin(), taggedNumbers.end()));
  auto doubledTaggedNumbers = lifted(taggedNumbersSource);
  
  TaggedValue<int, int> lastPushedValue;
  pull_fn pull = doubledTaggedNumbers(
    [&lastPushedValue](TaggedValue<int, int> v) { lastPushedValue = v; }
  );

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, lastPushedValue.value, "Lowered value should be transformed");
  TEST_ASSERT_EQUAL_MESSAGE(-1, lastPushedValue.tag, "Tagged value should remain intact");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(4, lastPushedValue.value, "Lowered value should be transformed");
  TEST_ASSERT_EQUAL_MESSAGE(-2, lastPushedValue.tag, "Tagged value should remain intact");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(6, lastPushedValue.value, "Lowered value should be transformed");
  TEST_ASSERT_EQUAL_MESSAGE(-3, lastPushedValue.tag, "Tagged value should remain intact");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_liftToOptional_can_push);
  RUN_TEST(test_liftToOptional_can_pull);
  RUN_TEST(test_liftToTaggedValue_lifts);
  UNITY_END();
}