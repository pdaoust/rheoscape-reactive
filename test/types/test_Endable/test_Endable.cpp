#include <unity.h>
#include <core_types.hpp>
#include <Endable.hpp>

using namespace rheo;

void test_Endable_has_value() {
  auto v = Endable(3);
  TEST_ASSERT_TRUE_MESSAGE(v.hasValue(), "v with indeterminate finality should have value");
  TEST_ASSERT_EQUAL_MESSAGE(3, v.value(), "v should have the correct value");

  v = Endable(4, false);
  TEST_ASSERT_TRUE_MESSAGE(v.hasValue(), "v with false finality should have value");
  TEST_ASSERT_EQUAL_MESSAGE(4, v.value(), "v should have the correct value");

  v = Endable(5, true);
  TEST_ASSERT_TRUE_MESSAGE(v.hasValue(), "v with true finality should have value");
  TEST_ASSERT_EQUAL_MESSAGE(5, v.value(), "v should have the correct value");
}

void test_Endable_has_no_value() {
  auto v = Endable<int>();

  TEST_ASSERT_FALSE_MESSAGE(v.hasValue(), "ended v should not have value");
  try {
    auto inner = v.value();
    TEST_FAIL_MESSAGE("should have thrown an exception on attempt to get value");
  } catch (endable_bad_get_value_access e) { }
}

void test_Endable_has_status() {
  auto v = Endable(3);
  TEST_ASSERT_EQUAL_MESSAGE(EndableStatus::Indeterminate, v.status(), "Should have unknown status");
  v = Endable(4, false);
  TEST_ASSERT_EQUAL_MESSAGE(EndableStatus::NotLast, v.status(), "Should not be the last");
  v = Endable(5, true);
  TEST_ASSERT_EQUAL_MESSAGE(EndableStatus::Last, v.status(), "Should be the last");
  v = Endable<int>();
  TEST_ASSERT_EQUAL_MESSAGE(EndableStatus::Ended, v.status(), "Should have ended status");
}

void test_Endable_knows_if_it_is_last() {
  auto v = Endable(3);
  TEST_ASSERT_EQUAL_MESSAGE(EndableIsLast::Unknowable, v.isLast(), "Shouldn't know if it's last");
  v = Endable(4, false);
  TEST_ASSERT_EQUAL_MESSAGE(EndableIsLast::No, v.status(), "Should not be the last");
  v = Endable(5, true);
  TEST_ASSERT_EQUAL_MESSAGE(EndableIsLast::Yes, v.status(), "Should be the last");
  v = Endable<int>();
  try {
    auto isLast = v.isLast();
    TEST_FAIL_MESSAGE("Should throw an exception when trying to find out if ended is last");
  } catch (endable_bad_get_value_access e) { }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_Endable_has_value);
  RUN_TEST(test_Endable_has_no_value);
  RUN_TEST(test_Endable_has_status);
  RUN_TEST(test_Endable_knows_if_it_is_last);
  UNITY_END();
}
