#include <unity.h>
#include <functional>
#include <operators/filter.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_filter_filters() {
  auto someNumbers = unwrapEndable(sequence(-3, 3, 1));
  auto filterer = filter(someNumbers, (filter_fn<int>)[](int v) { return v > 0; });
  std::optional<int> pushedValue;
  pull_fn pull = filterer([&pushedValue](int v) { pushedValue = v; });
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