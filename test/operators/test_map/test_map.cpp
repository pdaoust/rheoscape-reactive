#include <unity.h>
#include <functional>
#include <operators/map.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_map_maps() {
  auto some_numbers = unwrap_endable(sequence(-3, 3, 1));
  auto mapper = map(some_numbers, (map_fn<int, int>)[](int v) { return v * 2; });
  int pushed_value;
  pull_fn pull = mapper([&pushed_value](int v) { pushed_value = v; });
  for (int i = -3; i < 4; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i * 2, pushed_value, "");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_map_maps);
  UNITY_END();
}