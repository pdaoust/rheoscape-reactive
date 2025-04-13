#include <unity.h>
#include <functional>
#include <operators/map.hpp>
#include <operators/slowPwm.hpp>
#include <types/State.hpp>
#include <types/mock_clock.hpp>
#include <sources/fromClock.hpp>

void test_slowPwm_works() {
  auto clockSource = rheo::fromClock<rheo::mock_clock_ulong_millis>();
  auto clockSourceAsDuration = rheo::map<rheo::mock_clock_ulong_millis::duration, rheo::mock_clock_ulong_millis::time_point>(clockSource, [](rheo::mock_clock_ulong_millis::time_point v) { return v.time_since_epoch(); });
  rheo::State<float> dutySource(0.0f);
  auto pwm = rheo::slowPwm(dutySource.sourceFn(), clockSourceAsDuration, rheo::mock_clock_ulong_millis::duration(100));
  rheo::SwitchState pushedValue;
  auto pull = pwm(
    [&pushedValue](rheo::SwitchState v) { pushedValue = v; },
    [](){}
  );

  // Go through two duty cycles.
  for (int i = 0; i < 200; i ++) {
    rheo::mock_clock_ulong_millis::setTime(i);
    pull();
    TEST_ASSERT_TRUE_MESSAGE(pushedValue == rheo::SwitchState::off, "should be off through whole duty cycle");
  }

  dutySource.set(0.5f);
  for (int i = 0; i < 200; i ++) {
    rheo::mock_clock_ulong_millis::setTime(i);
    pull();
    if (i % 100 < 50) {
      std::string msg = "At " + std::to_string(i) + ", should be on through first half of duty cycle";
      TEST_ASSERT_TRUE_MESSAGE(pushedValue == rheo::SwitchState::on, msg.c_str());
    } else {
      std::string msg = "At " + std::to_string(i) + ", should be off through second half of duty cycle";
      TEST_ASSERT_TRUE_MESSAGE(pushedValue == rheo::SwitchState::off, msg.c_str());
    }
  }

  dutySource.set(1.0f);
  for (int i = 0; i < 200; i ++) {
    rheo::mock_clock_ulong_millis::setTime(i);
    pull();
    TEST_ASSERT_TRUE_MESSAGE(pushedValue == rheo::SwitchState::on, "should be on through whole duty cycle");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_slowPwm_works);
  UNITY_END();
}