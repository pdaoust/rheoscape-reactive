#include <unity.h>
#include <functional>
#include <types/State.hpp>
#include <operators/map.hpp>
#include <sources/sequence.hpp>

// There used to be an operator called `share`,
// which shared one source among many sinks.
// Turns out that's how normal source functions work.

void test_normal_source_function_pushes_to_all() {
  rheo::State<int> pushStream(0);
  auto source = pushStream.sourceFn();
  int pushedValue1 = 0;
  source([&pushedValue1](int v) { pushedValue1 = v; }, [](){});
  int pushedValue2 = 0;
  source([&pushedValue2](int v) { pushedValue2 = v; }, [](){});
  pushStream.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue1, "First sink should've gotten value");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue2, "Second sink should've gotten value");
}

void test_normal_source_function_only_pushes_to_puller() {
  auto pullStream = rheo::sequence(1, 2, 1);
  int pushedValue1 = 0;
  auto pull1 = pullStream([&pushedValue1](int v) { pushedValue1 = v; }, [](){});
  int pushedValue2 = 0;
  pullStream([&pushedValue2](int v) { pushedValue2 = v; }, [](){});
  pull1();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue1, "First sink should've gotten value");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(1, pushedValue2, "Second sink should not have gotten value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_normal_source_function_pushes_to_all);
  RUN_TEST(test_normal_source_function_only_pushes_to_puller);
  UNITY_END();
}