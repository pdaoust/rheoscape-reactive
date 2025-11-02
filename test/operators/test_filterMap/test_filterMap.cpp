#include <unity.h>
#include <functional>
#include <operators/filterMap.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_filterMap_filters_and_maps() {
  auto someNumbers = unwrapEndable(sequence(-3, 3, 1));
  auto filtered = filterMap(someNumbers, [](int v) {
    return v > 0
      ? std::optional(v * 2)
      : std::nullopt;
  });
  int pushedValue = INT_MAX;
  pull_fn pull = filtered([&pushedValue](int v) { pushedValue = v; });
  for (int i = -3; i <= 3; i ++) {
    pull();
    if (i > 0) {
      TEST_ASSERT_TRUE_MESSAGE(pushedValue > 0, "filterer should filter out everything not matched by the predicate");
      TEST_ASSERT_TRUE_MESSAGE(pushedValue % 2 == 0, "numbers should all be even after mapping");
    }
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_filterMap_filters_and_maps);
  UNITY_END();
}