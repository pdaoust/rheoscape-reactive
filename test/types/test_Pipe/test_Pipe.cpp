#include <unity.h>
#include <sources/fromIterator.hpp>
#include <operators/map.hpp>
#include <types/Pipe.hpp>

void test_Pipe_pipes_and_sinks() {
  std::vector<int> numbers { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  auto source = rheo::fromIterator(numbers.begin(), numbers.end());
  auto piper = rheo::Pipe<int>(source);

  int lastNumberPushed;
  auto pull = piper
    ._(rheo::map<int, int>([](int v) { return v * 2; }))
    ._(
      [&lastNumberPushed](int v) { lastNumberPushed = v; },
      [](){}
    );

  for (int i = 0; i < 10; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i * 2, lastNumberPushed, "Next number should've been pushed, then multiplied");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_Pipe_pipes_and_sinks);
  UNITY_END();
}
