#include <unity.h>
#include <functional>
#include <operators/filter.hpp>
#include <operators/combine.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>
#include <sources/constant.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_filter_filters() {
  auto some_numbers = unwrap_endable(sequence(-3, 3, 1));
  auto filterer = filter(some_numbers, (filter_fn<int>)[](int v) { return v > 0; });
  std::optional<int> pushed_value;
  pull_fn pull = filterer([&pushed_value](int v) { pushed_value = v; });
  for (int i = 0; i < 7; i ++) {
    pull();
    if (pushed_value.has_value()) {
      TEST_ASSERT_TRUE_MESSAGE(pushed_value > 0, "filterer should filter out everything not matched by the predicate");
    }
  }
}

void test_filter_with_tuple_unpacking() {
  source_fn<int> a = constant(3);
  source_fn<int> b = constant(5);
  auto combined = combine(a, b);

  // Filter with unpacked tuple args: keep only where x < y.
  auto filtered = filter(combined, [](int x, int y) { return x < y; });

  bool pushed = false;
  pull_fn pull = filtered([&pushed](std::tuple<int, int>) {
    pushed = true;
  });

  pull();
  TEST_ASSERT_TRUE_MESSAGE(pushed, "filter with tuple unpacking should pass (3, 5) since 3 < 5");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_filter_filters);
  RUN_TEST(test_filter_with_tuple_unpacking);
  UNITY_END();
}
