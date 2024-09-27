#include <unity.h>
#include <functional>
#include <operators/filter.hpp>
#include <sources/sequence.hpp>

void test_filter_filters() {
  auto someNumbers = rheo::sequence(-3, 3, 1);
  auto filterer = rheo::filter(someNumbers, (rheo::filter_fn<int>)[](int v) { return v > 0; });
  std::optional<int> pushedValue;
  rheo::pull_fn pull = filterer([&pushedValue](int v) { pushedValue = v; }, [](){});
  for (int i = 0; i < 7; i ++) {
    pull();
    if (pushedValue.has_value()) {
      TEST_ASSERT_TRUE_MESSAGE(pushedValue > 0, "filterer should filter out everything not matched by the predicate");
    }
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_filter_filters);
  UNITY_END();
}