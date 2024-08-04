#include <unity.h>
#include <operators/choose.hpp>
#include <sources/constant.hpp>
#include <types/State.hpp>

void test_choose_chooses_right_stream() {
  std::map<int, rheo::source_fn<int>> streams {
    { 1, rheo::constant(2) },
    { 2, rheo::constant(4) },
    { 3, rheo::constant(6) }
  };
  rheo::State<int> chooser(0);
  auto chooseStreams = rheo::choose<int, int>(streams, chooser.sourceFn());
  int lastPushedValue = 0;
  rheo::pull_fn pull = chooseStreams([&lastPushedValue](int v) { lastPushedValue = v; }, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, lastPushedValue, "Shouldn't have pushed a value if chooser's value isn't in the map");
  lastPushedValue = 0;
  chooser.set(1);
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, lastPushedValue, "Should have chosen the right stream's value from the map");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_choose_chooses_right_stream);
  UNITY_END();
}
