#include <unity.h>
#include <operators/concat.hpp>
#include <sources/fromIterator.hpp>

void test_concat_concats() {
  std::vector<int> first { 1, 2, 3 };
  auto firstStream = rheo::fromIterator(first.begin(), first.end());
  std::vector<int> second { 4, 5, 6 };
  auto secondStream = rheo::fromIterator(second.begin(), second.end());
  auto concatted = rheo::concat<int>(firstStream, secondStream);
  int lastPushedValue = 0;
  bool isEnded = false;
  rheo::pull_fn pull = concatted(
    [&lastPushedValue](int v) { lastPushedValue = v; },
    [&isEnded]() { isEnded = true; }
  );
  for (int i = 1; i <= 6; i ++) {
    lastPushedValue = 0;
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, lastPushedValue, "should get numbers 1 through 6");
  }
  TEST_ASSERT_TRUE_MESSAGE(isEnded, "should be ended after chewing through both streams");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_concat_concats);
  UNITY_END();
}
