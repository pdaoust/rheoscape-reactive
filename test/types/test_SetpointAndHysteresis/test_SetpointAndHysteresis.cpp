#include <unity.h>
#include <types/Range.hpp>

void test_SetpointAndHysteresis_knows_its_min_and_max() {
  rheo::SetpointAndHysteresis sph(20, 2);
  TEST_ASSERT_EQUAL_MESSAGE(18, sph.min(), "min should be setpoint - hysteresis");
  TEST_ASSERT_EQUAL_MESSAGE(22, sph.max(), "max should be setpoint + hysteresis");
}

void test_SetpointAndHysteresis_can_be_constructed_from_Range() {
  rheo::Range range(18, 22);
  rheo::SetpointAndHysteresis sph(range);
  TEST_ASSERT_EQUAL_MESSAGE(20, sph.setpoint, "setpoint should be halfway between range min and max");
  TEST_ASSERT_EQUAL_MESSAGE(2, sph.hysteresis, "hysteresis should be half of difference between range min and max");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_SetpointAndHysteresis_knows_its_min_and_max);
  RUN_TEST(test_SetpointAndHysteresis_can_be_constructed_from_Range);
  UNITY_END();
}
