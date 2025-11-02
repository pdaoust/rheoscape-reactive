#include <unity.h>
#include <util.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>
#include <sources/sequence.hpp>
#include <types/State.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_combine_combines_to_tuple() {
  source_fn<int> a = constant(3);
  source_fn<std::string> b = constant(std::string("hello"));
  auto a_b = combine(a, b, std::make_tuple<int, std::string>);
  std::tuple<int, std::string> pushedValue;
  pull_fn pull = a_b(
    [&pushedValue](auto v) {
      pushedValue = v;
    }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, std::get<0>(pushedValue), "should push a tuple and its first value should be the a value");
  TEST_ASSERT_TRUE_MESSAGE(std::get<1>(pushedValue) == "hello", "tuple's second value should be the b value");
}

void test_combine_combines_to_custom_value() {
  source_fn<int> a = constant(3);
  source_fn<int> b = constant(5);
  auto a_b = combine(a, b, [](int v_a, int v_b) { return v_a + v_b; });
  int pushedValue;
  pull_fn pull = a_b(
    [&pushedValue](auto v) {
      pushedValue = v;
    }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(8, pushedValue, "should combine two ints to sum of ints using custom combiner");
}

void test_combine_combines_on_push_from_push_source() {
  State<int> state(0);
  auto pushSource = state.sourceFn();
  auto normalSource = constant(3);
  auto combined = combine(pushSource, normalSource, std::make_tuple<int, int>);
  int pushedValueLeft = -1;
  int pushedValueRight = -1;
  int pushedCount = 0;
  combined(
    [&pushedValueLeft, &pushedValueRight, &pushedCount](std::tuple<int, int> v) {
      pushedValueLeft = std::get<0>(v);
      pushedValueRight = std::get<1>(v);
      pushedCount ++;
    }
  );
  state.set(5);
  TEST_ASSERT_EQUAL_MESSAGE(5, pushedValueLeft, "Should have pushed a value from the left source");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedValueRight, "Should have pushed a value from the right source");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "Should have pushed on upstream push");
}

void test_combine3_combines() {
  auto a = constant(1);
  auto b = constant('a');
  auto c = constant(true);
  auto combined = combine(a, b, c, std::make_tuple<int, char, bool>);
  auto pushedValue = std::make_tuple<int, char, bool>(-1, 'z', false);
  auto pull = combined(
    [&pushedValue](std::tuple<int, char, bool> v) { pushedValue = v; }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushedValue), "Value 1 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushedValue), "Value 2 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE(true, std::get<2>(pushedValue), "Value 3 should be in order");
}

void test_combine4_combines() {
  auto a = constant(1);
  auto b = constant('a');
  auto c = constant(true);
  auto d = constant(3.14f);
  auto combined = combine(a, b, c, d, std::make_tuple<int, char, bool, float>);
  auto pushedValue = std::make_tuple<int, char, bool, float>(-1, 'z', false, 9.999f);
  auto pull = combined(
    [&pushedValue](std::tuple<int, char, bool, float> v) { pushedValue = v; }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushedValue), "Value 1 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushedValue), "Value 2 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE(true, std::get<2>(pushedValue), "Value 3 should be in order");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(3.14f, std::get<3>(pushedValue), "Value 4 should be in order");
}

void test_combine_only_pulls_once_for_each_push() {
  // Two-operand combine
  auto a = sequenceOpen(1, 1);
  auto b = sequenceOpen('a', (char)1);
  auto combined2 = combine(a, b, std::make_tuple<int, char>);
  auto pushedValue2 = std::make_tuple<int, char>(-1, 'z');
  int pushCount2 = 0;
  auto pull2 = combined2([&pushedValue2, &pushCount2](std::tuple<int, char> v) {
    pushedValue2 = v;
    pushCount2 ++;
  });
  pull2();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushedValue2), "Should have combined the first value from source A");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushedValue2), "Should have combined the first value from source B");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushCount2, "Should only have pushed one value for each pull");

  // Three-operand combine
  auto c = sequenceOpen(2.0f, 0.5f);
  auto combined3 = combine(a, b, c, std::make_tuple<int, char, float>);
  auto pushedValue3 = std::make_tuple<int, char, float>(-1, 'z', 9.999f);
  int pushCount3 = 0;
  auto pull3 = combined3([&pushedValue3, &pushCount3](std::tuple<int, char, float> v) {
    pushedValue3 = v;
    pushCount3 ++;
  });
  pull3();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushedValue3), "Should have combined the first value from source A");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushedValue3), "Should have combined the first value from source B");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(2.0f, std::get<2>(pushedValue3), "Should have combined the first value from source C");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushCount3, "Should only have pushed one value for each pull");

  // Four-operand combine
  auto d = sequenceOpen<uint16_t>(3, 1);
  auto combined4 = combine(a, b, c, d, std::make_tuple<int, char, float, uint16_t>);
  auto pushedValue4 = std::make_tuple<int, char, float, int>(-1, 'z', 9.999f, 1000);
  int pushCount4 = 0;
  auto pull4 = combined4([&pushedValue4, &pushCount4](std::tuple<int, char, float, uint16_t> v) {
    pushedValue4 = v;
    pushCount4 ++;
  });
  pull4();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushedValue4), "Should have combined the first value from source A");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushedValue4), "Should have combined the first value from source B");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(2.0f, std::get<2>(pushedValue4), "Should have combined the first value from source C");
  TEST_ASSERT_EQUAL_MESSAGE(3, std::get<3>(pushedValue4), "Should have combined the first value from source D");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushCount4, "Should only have pushed one value for each pull");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_combine_combines_to_tuple);
  RUN_TEST(test_combine_combines_to_custom_value);
  RUN_TEST(test_combine_combines_on_push_from_push_source);
  RUN_TEST(test_combine3_combines);
  RUN_TEST(test_combine4_combines);
  RUN_TEST(test_combine_only_pulls_once_for_each_push);
  UNITY_END();
}
