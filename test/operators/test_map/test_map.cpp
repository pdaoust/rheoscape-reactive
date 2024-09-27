#include <unity.h>
#include <functional>
#include <operators/map.hpp>
#include <sources/sequence.hpp>

void test_map_maps() {
  auto someNumbers = rheo::sequence(-3, 3, 1);
  auto mapper = rheo::map(someNumbers, (rheo::map_fn<int, int>)[](int v) { return v * 2; });
  int pushedValue;
  rheo::pull_fn pull = mapper([&pushedValue](int v) { pushedValue = v; }, [](){});
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