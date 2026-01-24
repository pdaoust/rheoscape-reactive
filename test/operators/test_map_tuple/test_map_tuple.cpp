#include <unity.h>
#include <core_types.hpp>
#include <operators/map_tuple.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_map_tuple_transforms_tuple() {
  source_fn<int> a = constant(3);
  source_fn<int> b = constant(5);
  auto combined = combine(a, b);
  auto mapped = map_tuple(combined, [](int x, int y) { return x + y; });

  int pushed_value = 0;
  pull_fn pull = mapped([&pushed_value](int v) {
    pushed_value = v;
  });

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(8, pushed_value, "should transform tuple (3, 5) to 8 by summing");
}

void test_map_tuple_pipe_factory() {
  source_fn<int> a = constant(3);
  source_fn<std::string> b = constant(std::string("hello"));

  auto result = combine(a, b) | map_tuple([](int x, std::string s) {
    return s + std::to_string(x);
  });

  std::string pushed_value;
  pull_fn pull = result([&pushed_value](std::string v) {
    pushed_value = v;
  });

  pull();
  TEST_ASSERT_TRUE_MESSAGE(pushed_value == "hello3", "should transform via pipe operator");
}

void test_map_tuple_with_stateful_mapper() {
  source_fn<int> a = constant(10);
  source_fn<int> b = constant(5);
  auto combined = combine(a, b);

  int call_count = 0;
  auto mapped = map_tuple(combined, [&call_count](int x, int y) {
    call_count++;
    return x * y + call_count;
  });

  int pushed_value = 0;
  pull_fn pull = mapped([&pushed_value](int v) {
    pushed_value = v;
  });

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(51, pushed_value, "first call: 10*5 + 1 = 51");
  TEST_ASSERT_EQUAL_MESSAGE(1, call_count, "should have called mapper once");

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(52, pushed_value, "second call: 10*5 + 2 = 52");
  TEST_ASSERT_EQUAL_MESSAGE(2, call_count, "should have called mapper twice");
}

void test_map_tuple_with_three_elements() {
  source_fn<int> a = constant(1);
  source_fn<int> b = constant(2);
  source_fn<int> c = constant(3);
  auto combined = combine(a, b, c);

  auto mapped = map_tuple(combined, [](int x, int y, int z) {
    return x + y + z;
  });

  int pushed_value = 0;
  pull_fn pull = mapped([&pushed_value](int v) {
    pushed_value = v;
  });

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(6, pushed_value, "should sum 1+2+3 = 6");
}

void test_map_tuple_returns_different_type() {
  source_fn<int> a = constant(42);
  source_fn<int> b = constant(0);
  auto combined = combine(a, b);

  auto mapped = map_tuple(combined, [](int x, int y) {
    return x > y;
  });

  bool pushed_value = false;
  pull_fn pull = mapped([&pushed_value](bool v) {
    pushed_value = v;
  });

  pull();
  TEST_ASSERT_TRUE_MESSAGE(pushed_value, "should return true for 42 > 0");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_map_tuple_transforms_tuple);
  RUN_TEST(test_map_tuple_pipe_factory);
  RUN_TEST(test_map_tuple_with_stateful_mapper);
  RUN_TEST(test_map_tuple_with_three_elements);
  RUN_TEST(test_map_tuple_returns_different_type);
  UNITY_END();
}
