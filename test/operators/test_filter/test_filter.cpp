#include <unity.h>
#include <functional>
#include <operators/filter.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

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

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_filter_filters);
  UNITY_END();
}