#include <unity.h>
#include <util.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>
#include <sources/sequence.hpp>
#include <types/State.hpp>
#include <types/EventSource.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_combine_combines_to_tuple() {
  source_fn<int> a = constant(3);
  source_fn<std::string> b = constant(std::string("hello"));
  auto a_b = combine(a, b);
  std::tuple<int, std::string> pushed_value;
  pull_fn pull = a_b(
    [&pushed_value](auto v) {
      pushed_value = v;
    }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, std::get<0>(pushed_value), "should push a tuple and its first value should be the a value");
  TEST_ASSERT_TRUE_MESSAGE(std::get<1>(pushed_value) == "hello", "tuple's second value should be the b value");
}

void test_combine_combines_on_push_from_push_source() {
  State<int> state(0);
  auto push_source = state.get_source_fn();
  auto normal_source = constant(3);
  auto combined = combine(push_source, normal_source);
  int pushed_value_left = -1;
  int pushed_value_right = -1;
  int pushed_count = 0;
  combined(
    [&pushed_value_left, &pushed_value_right, &pushed_count](std::tuple<int, int> v) {
      pushed_value_left = std::get<0>(v);
      pushed_value_right = std::get<1>(v);
      pushed_count ++;
    }
  );
  state.set(5);
  TEST_ASSERT_EQUAL_MESSAGE(5, pushed_value_left, "Should have pushed a value from the left source");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_value_right, "Should have pushed a value from the right source");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should have pushed on upstream push");
}

void test_combine3_combines() {
  auto a = constant(1);
  auto b = constant('a');
  auto c = constant(true);
  auto combined = combine(a, b, c);
  auto pushed_value = std::make_tuple<int, char, bool>(-1, 'z', false);
  auto pull = combined(
    [&pushed_value](std::tuple<int, char, bool> v) { pushed_value = v; }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value), "Value 1 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value), "Value 2 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE(true, std::get<2>(pushed_value), "Value 3 should be in order");
}

void test_combine4_combines() {
  auto a = constant(1);
  auto b = constant('a');
  auto c = constant(true);
  auto d = constant(3.14f);
  auto combined = combine(a, b, c, d);
  auto pushed_value = std::make_tuple<int, char, bool, float>(-1, 'z', false, 9.999f);
  auto pull = combined(
    [&pushed_value](std::tuple<int, char, bool, float> v) { pushed_value = v; }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value), "Value 1 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value), "Value 2 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE(true, std::get<2>(pushed_value), "Value 3 should be in order");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(3.14f, std::get<3>(pushed_value), "Value 4 should be in order");
}

void test_combine_only_pulls_once_for_each_push() {
  // Two-operand combine
  auto a = sequence_open(1, 1);
  auto b = sequence_open('a', (char)1);
  auto combined2 = combine(a, b);
  auto pushed_value2 = std::make_tuple<int, char>(-1, 'z');
  int push_count2 = 0;
  auto pull2 = combined2([&pushed_value2, &push_count2](std::tuple<int, char> v) {
    pushed_value2 = v;
    push_count2 ++;
  });
  pull2();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value2), "Should have combined the first value from source A");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value2), "Should have combined the first value from source B");
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count2, "Should only have pushed one value for each pull");

  // Three-operand combine
  auto c = sequence_open(2.0f, 0.5f);
  auto combined3 = combine(a, b, c);
  auto pushed_value3 = std::make_tuple<int, char, float>(-1, 'z', 9.999f);
  int push_count3 = 0;
  auto pull3 = combined3([&pushed_value3, &push_count3](std::tuple<int, char, float> v) {
    pushed_value3 = v;
    push_count3 ++;
  });
  pull3();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value3), "Should have combined the first value from source A");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value3), "Should have combined the first value from source B");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(2.0f, std::get<2>(pushed_value3), "Should have combined the first value from source C");
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count3, "Should only have pushed one value for each pull");

  // Four-operand combine
  auto d = sequence_open<uint16_t>(3, 1);
  auto combined4 = combine(a, b, c, d);
  auto pushed_value4 = std::make_tuple<int, char, float, int>(-1, 'z', 9.999f, 1000);
  int push_count4 = 0;
  auto pull4 = combined4([&pushed_value4, &push_count4](std::tuple<int, char, float, uint16_t> v) {
    pushed_value4 = v;
    push_count4 ++;
  });
  pull4();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value4), "Should have combined the first value from source A");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value4), "Should have combined the first value from source B");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(2.0f, std::get<2>(pushed_value4), "Should have combined the first value from source C");
  TEST_ASSERT_EQUAL_MESSAGE(3, std::get<3>(pushed_value4), "Should have combined the first value from source D");
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count4, "Should only have pushed one value for each pull");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_combine_combines_to_tuple);
  RUN_TEST(test_combine_combines_on_push_from_push_source);
  RUN_TEST(test_combine3_combines);
  RUN_TEST(test_combine4_combines);
  RUN_TEST(test_combine_only_pulls_once_for_each_push);
  UNITY_END();
}
