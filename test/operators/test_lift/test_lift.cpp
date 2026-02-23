#include <unity.h>
#include <functional>
#include <operators/lift.hpp>
#include <operators/map.hpp>
#include <operators/unwrap.hpp>
#include <states/MemoryState.hpp>
#include <sources/from_iterator.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_lift_to_optional_can_push() {
  pipe_fn<int, int> doubling_pipe = map([](int v) { return v * 2; });
  auto lifted = lift_to_optional(doubling_pipe);
  MemoryState<std::optional<int>> state;
  auto doubled_optional_numbers = lifted(state.get_source_fn());

  std::optional<int> last_pushed_value = std::nullopt;
  doubled_optional_numbers(
    [&last_pushed_value](std::optional<int> v) { last_pushed_value = v; }
  );

  state.set(3);
  TEST_ASSERT_TRUE_MESSAGE(last_pushed_value.has_value(), "Lowerable should get pushed");
  TEST_ASSERT_EQUAL_MESSAGE(6, last_pushed_value.value(), "Value should get transformed by inner pipe");
  state.set(std::nullopt);
  TEST_ASSERT_FALSE_MESSAGE(last_pushed_value.has_value(), "Non-lowerable value should get pushed too");
}

void test_lift_to_optional_can_pull() {
  // Try lifting a mapping pipe that knows nothing about optionals
  // to a mapping pipe that does.
  pipe_fn<int, int> doubling_pipe = map([](int v) { return v * 2; });
  auto lifted = lift_to_optional(doubling_pipe);
  std::vector<std::optional<int>> optional_numbers {
    3,
    std::nullopt,
    1,
    5,
    std::nullopt
  };
  auto optional_numbers_source = unwrap_endable(from_iterator(optional_numbers.begin(), optional_numbers.end()));
  auto doubled_optional_numbers = lifted(optional_numbers_source);

  int pushed_count = 0;
  std::optional<int> last_pushed_value = 0;
  pull_fn pull = doubled_optional_numbers(
    [&pushed_count, &last_pushed_value](std::optional<int> v) {
      pushed_count ++;
      last_pushed_value = v;
    }
  );

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should push on non-empty pulls only");
  TEST_ASSERT_TRUE_MESSAGE(last_pushed_value.has_value(), "Should have pushed value for non-empty");
  TEST_ASSERT_EQUAL_MESSAGE(6, last_pushed_value.value(), "Should have doubled the non-empty value");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_count, "Should not push on empty pulls");
  TEST_ASSERT_FALSE_MESSAGE(last_pushed_value.has_value(), "Should have pushed empty value for empty");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_count, "Should push on non-empty pulls only");
  TEST_ASSERT_TRUE_MESSAGE(last_pushed_value.has_value(), "Should have pushed value for non-empty");
  TEST_ASSERT_EQUAL_MESSAGE(2, last_pushed_value.value(), "Should have doubled the non-empty value");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(4, pushed_count, "Should push on non-empty pulls only");
  TEST_ASSERT_TRUE_MESSAGE(last_pushed_value.has_value(), "Should have pushed value for non-empty");
  TEST_ASSERT_EQUAL_MESSAGE(10, last_pushed_value.value(), "Should have doubled the non-empty value");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(5, pushed_count, "Should not push on empty pulls");
TEST_ASSERT_FALSE_MESSAGE(last_pushed_value.has_value(), "Should have pushed empty value for empty");
}

void test_lift_to_taggedValue_lifts() {
  pipe_fn<int, int> doubling_pipe = map([](int v) { return v * 2; });
  auto lifted = lift_to_tagged_value<int>(doubling_pipe);
  std::vector<std::tuple<int, int>> tagged_numbers {
    std::tuple { 1, -1 },
    std::tuple { 2, -2 },
    std::tuple { 3, -3 },
  };
  auto tagged_numbers_source = unwrap_endable(from_iterator(tagged_numbers.begin(), tagged_numbers.end()));
  auto doubled_tagged_numbers = lifted(tagged_numbers_source);

  std::tuple<int, int> last_pushed_value;
  pull_fn pull = doubled_tagged_numbers(
    [&last_pushed_value](std::tuple<int, int> v) { last_pushed_value = v; }
  );

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, last_pushed_value.value, "Lowered value should be transformed");
  TEST_ASSERT_EQUAL_MESSAGE(-1, last_pushed_value.tag, "Tagged value should remain intact");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(4, last_pushed_value.value, "Lowered value should be transformed");
  TEST_ASSERT_EQUAL_MESSAGE(-2, last_pushed_value.tag, "Tagged value should remain intact");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(6, last_pushed_value.value, "Lowered value should be transformed");
  TEST_ASSERT_EQUAL_MESSAGE(-3, last_pushed_value.tag, "Tagged value should remain intact");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_lift_to_optional_can_push);
  RUN_TEST(test_lift_to_optional_can_pull);
  RUN_TEST(test_lift_to_taggedValue_lifts);
  UNITY_END();
}