#include <unity.h>
#include <types/mock_clock.hpp>

void test_mock_clock_can_get_time() {
  TEST_ASSERT_EQUAL(0, rheo::mock_clock<int>::now().time_since_epoch().count());
}

void test_mock_clock_can_set_time() {
  rheo::mock_clock<int>::setTime(15);
  TEST_ASSERT_EQUAL(15, rheo::mock_clock<int>::now().time_since_epoch().count());
}

void test_mock_clock_can_tick() {
  rheo::mock_clock<int>::setTime(0);
  for (int i = 0; i < 15; i ++) {
    TEST_ASSERT_EQUAL(i, rheo::mock_clock<int>::now().time_since_epoch().count());
    rheo::mock_clock<int>::tick();
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_mock_clock_can_get_time);
  RUN_TEST(test_mock_clock_can_set_time);
  RUN_TEST(test_mock_clock_can_tick);
  UNITY_END();
}
