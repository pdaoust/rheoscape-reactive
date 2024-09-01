#include <unity.h>
#include <util.hpp>
#include <operators/zip.hpp>
#include <sources/constant.hpp>

void test_zip_zips_to_tuple() {
  rheo::source_fn<int> a = rheo::constant(3);
  rheo::source_fn<std::string> b = rheo::constant(std::string("hello"));
  auto a_b = rheo::zipTuple(a, b);
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

void test_zip_zips_to_custom_value() {
  rheo::source_fn<int> a = rheo::constant(3);
  rheo::source_fn<int> b = rheo::constant(5);
  auto a_b = rheo::zip<int, int, int>(a, b, [](int v_a, int v_b) { return v_a + v_b; });
  int pushedValue;
  rheo::pull_fn pull = a_b(
    [&pushedValue](auto v) {
      pushedValue = v;
    },
    [](){}
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(8, pushedValue, "should zip two ints to sum of ints using custom zipper");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_zip_zips_to_tuple);
  RUN_TEST(test_zip_zips_to_custom_value);
  UNITY_END();
}
