#include <unity.h>
#include <chrono>
#include <types/mock_clock.hpp>

void test_mock_clock_can_get_time() {
  auto now = rheo::mock_clock<int, std::milli>::now();
  TEST_ASSERT_EQUAL_MESSAGE(0, now.time_since_epoch().count(), "should get starting time for clock");
}

void test_mock_clock_can_set_time() {
  rheo::mock_clock<int, std::milli>::setTime(15);
  auto now = rheo::mock_clock<int, std::milli>::now();
  TEST_ASSERT_EQUAL_MESSAGE(15, now.time_since_epoch().count(), "should get the set time");
}

void test_mock_clock_can_tick() {
  rheo::mock_clock<int, std::milli>::setTime(0);
  for (int i = 0; i < 15; i ++) {
    auto now = rheo::mock_clock<int, std::milli>::now();
    TEST_ASSERT_EQUAL_MESSAGE(i, now.time_since_epoch().count(), "should tick along merrily");
    rheo::mock_clock<int, std::milli>::tick();
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_mock_clock_can_get_time);
  RUN_TEST(test_mock_clock_can_set_time);
  RUN_TEST(test_mock_clock_can_tick);
  UNITY_END();
}
