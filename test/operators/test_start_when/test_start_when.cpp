#include <unity.h>
#include <functional>
#include <operators/start_when.hpp>
#include <operators/unwrap.hpp>
#include <sources/from_iterator.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_start_when_starts_when() {
  std::vector<int> numbers { 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4 };
  auto numbers_source = unwrap_endable(from_iterator(numbers.begin(), numbers.end()));
  auto numbers_above_four = start_when(numbers_source, (filter_fn<int>)[](int v) { return v > 4; });
  int pushed_value = -1;
  auto pull = numbers_above_four(
    [&pushed_value](int v) { pushed_value = v; }
  );
  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(-1, pushed_value, "Shouldn't start as long as values are below four");
  }
  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_TRUE_MESSAGE(pushed_value > -1, "Should deliver a bunch of numbers after condition is hit");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(4, pushed_value, "Should deliver a value that doesn't match the condition, but only after the condition has been matched once");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_start_when_starts_when);
  UNITY_END();
}