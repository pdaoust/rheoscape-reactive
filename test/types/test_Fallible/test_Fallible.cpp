#include <string>
#include <unity.h>
#include <core_types.hpp>
#include <Fallible.hpp>

using namespace std::string_literals;
using namespace rheo;

void test_Fallible_has_value() {
  auto v = Fallible<int, std::string>(3);
  TEST_ASSERT_TRUE_MESSAGE(v.isOk(), "Non-error fallible should be OK");
  TEST_ASSERT_FALSE_MESSAGE(v.isError(), "Non-error fallible should not be an error");
  TEST_ASSERT_EQUAL_MESSAGE(3, v.value(), "Fallible should have the correct value");
  try {
    auto err = v.error();
    TEST_FAIL_MESSAGE("Attempt to get error should've failed");
  } catch (fallible_bad_get_error_access e) {}
}

void test_Fallible_has_error() {
  auto v = Fallible<int, std::string>("Error: it failed!"s);
  TEST_ASSERT_FALSE_MESSAGE(v.isOk(), "Error fallible should not be OK");
  TEST_ASSERT_TRUE_MESSAGE(v.isError(), "Error fallible should be an error");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("Error: it failed!", v.error().c_str(), "Fallible should have the correct error value");
  try {
    auto value = v.value();
    TEST_FAIL_MESSAGE("Attempt to get value should've failed");
  } catch (fallible_bad_get_value_access e) {}
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_Fallible_has_value);
  RUN_TEST(test_Fallible_has_error);
  UNITY_END();
}
