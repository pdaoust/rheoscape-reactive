#include <unity.h>
#include <functional>
#include <operators/lift.hpp>
#include <operators/map.hpp>
#include <types/State.hpp>
#include <sources/fromIterator.hpp>

void test_liftToOptional_can_push() {
  auto doublingPipe = rheo::map<int, int>([](int v) { return v * 2; });
  auto lifted = rheo::liftToOptional(doublingPipe);
  rheo::State<std::optional<int>> state;
  auto doubledOptionalNumbers = lifted(state.sourceFn());

  std::optional<int> lastPushedValue = std::nullopt;
  doubledOptionalNumbers(
    [&lastPushedValue](std::optional<int> v) { lastPushedValue = v; },
    [](){}
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
  auto doublingPipe = rheo::map<int, int>([](int v) { return v * 2; });
  auto lifted = rheo::liftToOptional(doublingPipe);
  std::vector<std::optional<int>> optionalNumbers {
    3,
    std::nullopt,
    1,
    5,
    std::nullopt
  };
  auto optionalNumbersSource = rheo::fromIterator(optionalNumbers.begin(), optionalNumbers.end());
  auto doubledOptionalNumbers = lifted(optionalNumbersSource);

  int pushedCount = 0;
  std::optional<int> lastPushedValue = 0;
  rheo::pull_fn pull = doubledOptionalNumbers(
    [&pushedCount, &lastPushedValue](std::optional<int> v) {
      pushedCount ++;
      lastPushedValue = v;
    },
    [](){}
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

void test_liftToOptional_can_end() {
  auto doublingPipe = rheo::map<int, int>([](int v) { return v * 2; });
  auto lifted = rheo::liftToOptional(doublingPipe);
  std::vector<std::optional<int>> optionalNumbers {
    3,
    std::nullopt,
    1,
    5,
    std::nullopt
  };
  auto optionalNumbersSource = rheo::fromIterator(optionalNumbers.begin(), optionalNumbers.end());
  auto doubledOptionalNumbers = lifted(optionalNumbersSource);

  bool isEnded = false;
  rheo::pull_fn pull = doubledOptionalNumbers(
    [](std::optional<int> v) { },
    [&isEnded](){ isEnded = true; }
  );

  pull();
  pull();
  pull();
  pull();
  TEST_ASSERT_FALSE_MESSAGE(isEnded, "Shouldn't be ended yet");
  pull();
  TEST_ASSERT_TRUE_MESSAGE(isEnded, "Should be ended now");
}

void test_liftToTaggedValue_lifts() {
  auto doublingPipe = rheo::map<int, int>([](int v) { return v * 2; });
  auto lifted = rheo::liftToTaggedValue<int>(doublingPipe);
  std::vector<rheo::TaggedValue<int, int>> taggedNumbers {
    rheo::TaggedValue { 1, -1 },
    rheo::TaggedValue { 2, -2 },
    rheo::TaggedValue { 3, -3 },
  };
  auto taggedNumbersSource = rheo::fromIterator(taggedNumbers.begin(), taggedNumbers.end());
  auto doubledTaggedNumbers = lifted(taggedNumbersSource);
  
  rheo::TaggedValue<int, int> lastPushedValue;
  rheo::pull_fn pull = doubledTaggedNumbers(
    [&lastPushedValue](rheo::TaggedValue<int, int> v) { lastPushedValue = v; },
    [](){}
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
  RUN_TEST(test_liftToOptional_can_end);
  RUN_TEST(test_liftToTaggedValue_lifts);
  UNITY_END();
}