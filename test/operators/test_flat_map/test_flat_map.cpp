#include <unity.h>
#include <functional>
#include <operators/flat_map.hpp>
#include <operators/unwrap.hpp>
#include <sources/constant.hpp>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_flat_map_expands_values() {
  auto source = constant(3);
  auto expanded = flat_map(source, [](int v) {
    return std::vector<int>(v, v);
  });

  std::vector<int> pushed_values;
  pull_fn pull = expanded([&pushed_values](int v) { pushed_values.push_back(v); });

  pull();
  TEST_ASSERT_EQUAL(3, pushed_values.size());
  TEST_ASSERT_EQUAL(3, pushed_values[0]);
  TEST_ASSERT_EQUAL(3, pushed_values[1]);
  TEST_ASSERT_EQUAL(3, pushed_values[2]);
}

void test_flat_map_changes_output_type() {
  auto source = constant(5);
  auto mapped = flat_map(source, [](int v) {
    return std::vector<std::string>(v, "x");
  });

  std::vector<std::string> pushed_values;
  pull_fn pull = mapped([&pushed_values](std::string v) { pushed_values.push_back(v); });

  pull();
  TEST_ASSERT_EQUAL(5, pushed_values.size());
  for (auto& s : pushed_values) {
    TEST_ASSERT_TRUE(s == "x");
  }
}

void test_flat_map_empty_vector_pushes_nothing() {
  auto source = constant(42);
  auto mapped = flat_map(source, [](int) {
    return std::vector<int>{};
  });

  int push_count = 0;
  pull_fn pull = mapped([&push_count](int) { push_count++; });

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, push_count, "empty vector should push nothing");

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, push_count, "still nothing on second pull");
}

void test_flat_map_variable_output_count() {
  auto source = unwrap_endable(sequence(0, 3, 1));
  // Each input n produces n copies of n.
  auto mapped = flat_map(source, [](int n) {
    return std::vector<int>(n, n);
  });

  std::vector<int> pushed_values;
  pull_fn pull = mapped([&pushed_values](int v) { pushed_values.push_back(v); });

  // n=0: pushes nothing
  pull();
  TEST_ASSERT_EQUAL(0, pushed_values.size());

  // n=1: pushes {1}
  pull();
  TEST_ASSERT_EQUAL(1, pushed_values.size());
  TEST_ASSERT_EQUAL(1, pushed_values[0]);

  // n=2: pushes {2, 2}
  pull();
  TEST_ASSERT_EQUAL(3, pushed_values.size());
  TEST_ASSERT_EQUAL(2, pushed_values[1]);
  TEST_ASSERT_EQUAL(2, pushed_values[2]);

  // n=3: pushes {3, 3, 3}
  pull();
  TEST_ASSERT_EQUAL(6, pushed_values.size());
  TEST_ASSERT_EQUAL(3, pushed_values[3]);
  TEST_ASSERT_EQUAL(3, pushed_values[4]);
  TEST_ASSERT_EQUAL(3, pushed_values[5]);
}

void test_flat_map_pipe_factory() {
  auto result = constant(2) | flat_map([](int v) {
    return std::vector<int>{v, v * 10, v * 100};
  });

  std::vector<int> pushed_values;
  pull_fn pull = result([&pushed_values](int v) { pushed_values.push_back(v); });

  pull();
  TEST_ASSERT_EQUAL(3, pushed_values.size());
  TEST_ASSERT_EQUAL(2, pushed_values[0]);
  TEST_ASSERT_EQUAL(20, pushed_values[1]);
  TEST_ASSERT_EQUAL(200, pushed_values[2]);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_flat_map_expands_values);
  RUN_TEST(test_flat_map_changes_output_type);
  RUN_TEST(test_flat_map_empty_vector_pushes_nothing);
  RUN_TEST(test_flat_map_variable_output_count);
  RUN_TEST(test_flat_map_pipe_factory);
  UNITY_END();
}
