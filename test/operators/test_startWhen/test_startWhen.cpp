#include <unity.h>
#include <functional>
#include <operators/startWhen.hpp>
#include <operators/unwrap.hpp>
#include <sources/fromIterator.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_startWhen_starts_when() {
  std::vector<int> numbers { 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4 };
  auto numbersSource = unwrapEndable(fromIterator(numbers.begin(), numbers.end()));
  auto numbersAboveFour = startWhen(numbersSource, (filter_fn<int>)[](int v) { return v > 4; });
  int pushedValue = -1;
  auto pull = numbersAboveFour(
    [&pushedValue](int v) { pushedValue = v; }
  );
  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(-1, pushedValue, "Shouldn't start as long as values are below four");
  }
  for (int i = 0; i < 5; i ++) {
    pull();
    TEST_ASSERT_TRUE_MESSAGE(pushedValue > -1, "Should deliver a bunch of numbers after condition is hit");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(4, pushedValue, "Should deliver a value that doesn't match the condition, but only after the condition has been matched once");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_startWhen_starts_when);
  UNITY_END();
}