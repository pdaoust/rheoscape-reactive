#include <unity.h>
#include <functional>
#include <states/MemoryState.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheoscape;
using namespace rheoscape::sources;
using namespace rheoscape::operators;

// There used to be an operator called `share`,
// which shared one source among many sinks.
// Turns out that's how normal source functions work.

void test_normal_source_function_pushes_to_all() {
  MemoryState<int> push_stream(0);
  auto source = push_stream.get_source_fn();
  int pushed_value1 = 0;
  source([&pushed_value1](int v) { pushed_value1 = v; });
  int pushed_value2 = 0;
  source([&pushed_value2](int v) { pushed_value2 = v; });
  push_stream.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value1, "First sink should've gotten value");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value2, "Second sink should've gotten value");
}

void test_normal_source_function_only_pushes_to_puller() {
  auto pull_stream = unwrap_endable(sequence(1, 2, 1));
  int pushed_value1 = 0;
  auto pull1 = pull_stream([&pushed_value1](int v) { pushed_value1 = v; });
  int pushed_value2 = 0;
  pull_stream([&pushed_value2](int v) { pushed_value2 = v; });
  pull1();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value1, "First sink should've gotten value");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(1, pushed_value2, "Second sink should not have gotten value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_normal_source_function_pushes_to_all);
  RUN_TEST(test_normal_source_function_only_pushes_to_puller);
  UNITY_END();
}