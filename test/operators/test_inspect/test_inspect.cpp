#include <unity.h>
#include <operators/inspect.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_inspect_with_tuple_unpacking() {
  source_fn<int> a = constant(3);
  source_fn<int> b = constant(5);
  auto combined = combine(a, b);

  int inspected_sum = 0;
  auto inspected = inspect(combined, [&inspected_sum](int x, int y) {
    inspected_sum = x + y;
  });

  std::tuple<int, int> pushed_value;
  pull_fn pull = inspected([&pushed_value](std::tuple<int, int> v) {
    pushed_value = v;
  });

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(8, inspected_sum, "inspect should unpack tuple for callback");
  TEST_ASSERT_EQUAL_MESSAGE(3, std::get<0>(pushed_value), "value should pass through unchanged");
  TEST_ASSERT_EQUAL_MESSAGE(5, std::get<1>(pushed_value), "value should pass through unchanged");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_inspect_with_tuple_unpacking);
  UNITY_END();
}
