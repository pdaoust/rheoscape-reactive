#include <unity.h>
#include <util.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>
#include <types/State.hpp>

void test_combine_combines_to_tuple() {
  rheo::source_fn<int> a = rheo::constant(3);
  rheo::source_fn<std::string> b = rheo::constant(std::string("hello"));
  auto a_b = rheo::combineTuple(a, b);
  std::tuple<int, std::string> pushedValue;
  rheo::pull_fn pull = a_b(
    [&pushedValue](auto v) {
      pushedValue = v;
    },
    [](){}
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, std::get<0>(pushedValue), "should push a tuple and its first value should be the a value");
  TEST_ASSERT_TRUE_MESSAGE(std::get<1>(pushedValue) == "hello", "tuple's second value should be the b value");
}

void test_combine_combines_to_custom_value() {
  rheo::source_fn<int> a = rheo::constant(3);
  rheo::source_fn<int> b = rheo::constant(5);
  auto a_b = rheo::combine<int, int, int>(a, b, [](int v_a, int v_b) { return v_a + v_b; });
  int pushedValue;
  rheo::pull_fn pull = a_b(
    [&pushedValue](auto v) {
      pushedValue = v;
    },
    [](){}
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(8, pushedValue, "should combine two ints to sum of ints using custom combiner");
}

void test_combine_combines_on_push_from_push_source() {
  rheo::State<int> state(0);
  auto pushSource = state.sourceFn();
  auto normalSource = rheo::constant(3);
  auto combined = rheo::combineTuple(pushSource, normalSource);
  int pushedValueLeft = -1;
  int pushedValueRight = -1;
  int pushedCount = 0;
  combined(
    [&pushedValueLeft, &pushedValueRight, &pushedCount](std::tuple<int, int> v) {
      pushedValueLeft = std::get<0>(v);
      pushedValueRight = std::get<1>(v);
      pushedCount ++;
    },
    [](){}
  );
  state.set(5);
  TEST_ASSERT_EQUAL_MESSAGE(5, pushedValueLeft, "Should have pushed a value from the left source");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedValueRight, "Should have pushed a value from the right source");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should have pushed on upstream push");
}

void test_combine3_combines() {
  auto a = rheo::constant(1);
  auto b = rheo::constant('a');
  auto c = rheo::constant(false);
  auto combined = rheo::combine3Tuple(a, b, c);
  std::tuple<int, char, bool> pushedValue;
  auto pull = combined(
    [&pushedValue](std::tuple<int, char, bool> v) { pushedValue = v; },
    [](){}
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushedValue), "Value 1 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushedValue), "Value 2 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE(false, std::get<2>(pushedValue), "Value 3 should be in order");
}

void test_combine4_combines() {
  auto a = rheo::constant(1);
  auto b = rheo::constant('a');
  auto c = rheo::constant(false);
  auto d = rheo::constant(3.14f);
  auto combined = rheo::combine4Tuple(a, b, c, d);
  std::tuple<int, char, bool, float> pushedValue;
  auto pull = combined(
    [&pushedValue](std::tuple<int, char, bool, float> v) { pushedValue = v; },
    [](){}
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushedValue), "Value 1 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushedValue), "Value 2 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE(false, std::get<2>(pushedValue), "Value 3 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE(3.14f, std::get<3>(pushedValue), "Value 4 should be in order");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_combine_combines_to_tuple);
  RUN_TEST(test_combine_combines_to_custom_value);
  RUN_TEST(test_combine_combines_on_push_from_push_source);
  RUN_TEST(test_combine3_combines);
  RUN_TEST(test_combine4_combines);
  UNITY_END();
}
