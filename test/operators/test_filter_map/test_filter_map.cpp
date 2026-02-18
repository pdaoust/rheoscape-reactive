#include <unity.h>
#include <functional>
#include <operators/filter_map.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_filter_map_filters_and_maps() {
  auto some_numbers = unwrap_endable(sequence(-3, 3, 1));
  auto filtered = filter_map(some_numbers, [](int v) {
    return v > 0
      ? std::optional(v * 2)
      : std::nullopt;
  });
  int pushed_value = INT_MAX;
  pull_fn pull = filtered([&pushed_value](int v) { pushed_value = v; });
  for (int i = -3; i <= 3; i ++) {
    pull();
    if (i > 0) {
      TEST_ASSERT_TRUE_MESSAGE(pushed_value > 0, "filterer should filter out everything not matched by the predicate");
      TEST_ASSERT_TRUE_MESSAGE(pushed_value % 2 == 0, "numbers should all be even after mapping");
    }
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_filter_map_filters_and_maps);
  UNITY_END();
}