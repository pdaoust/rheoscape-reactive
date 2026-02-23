#include <unity.h>
#include <operators/choose.hpp>
#include <sources/constant.hpp>
#include <states/MemoryState.hpp>

using namespace rheoscape;
using namespace rheoscape::sources;
using namespace rheoscape::operators;

void test_choose_chooses_right_stream() {
  std::map<int, source_fn<int>> streams {
    { 1, constant(2) },
    { 2, constant(4) },
    { 3, constant(6) }
  };
  MemoryState<int> chooser(0);
  auto choose_streams = choose(streams, chooser.get_source_fn());
  int last_pushed_value = 0;
  pull_fn pull = choose_streams([&last_pushed_value](int v) { last_pushed_value = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, last_pushed_value, "Shouldn't have pushed a value if chooser's value isn't in the map");
  last_pushed_value = 0;
  chooser.set(1);
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, last_pushed_value, "Should have chosen the right stream's value from the map");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_choose_chooses_right_stream);
  UNITY_END();
}
