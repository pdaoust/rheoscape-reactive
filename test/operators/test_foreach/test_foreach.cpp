#include <unity.h>
#include <fmt/format.h>
#include <operators/foreach.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_foreach_is_called_for_each() {
  int pushed_count = 0;
  auto source = constant(true);
  auto pull = source | foreach([&pushed_count](bool _) { pushed_count ++; });
  for (int i = 1; i < 10; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_count, fmt::format("Should have pushed {} times", i).c_str());
  }
}

void test_foreach_with_tuple_unpacking() {
  source_fn<int> a = constant(3);
  source_fn<int> b = constant(5);
  auto combined = combine(a, b);

  int sum = 0;
  auto pull = combined | foreach([&sum](int x, int y) { sum = x + y; });

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(8, sum, "foreach with tuple unpacking should unpack (3, 5)");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_foreach_is_called_for_each);
  RUN_TEST(test_foreach_with_tuple_unpacking);
  UNITY_END();
}
