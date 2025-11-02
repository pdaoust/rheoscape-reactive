#include <unity.h>
#include <functional>
#include <operators/map.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_map_maps() {
  auto someNumbers = unwrapEndable(sequence(-3, 3, 1));
  auto mapper = map(someNumbers, (map_fn<int, int>)[](int v) { return v * 2; });
  int pushedValue;
  pull_fn pull = mapper([&pushedValue](int v) { pushedValue = v; });
  for (int i = -3; i < 4; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i * 2, pushedValue, "");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_map_maps);
  UNITY_END();
}