#include <unity.h>
#include <operators/bangBang.hpp>
#include <sources/constant.hpp>
#include <sources/sequence.hpp>
#include <types/State.hpp>

void test_bangBang_goes_up() {
  auto sph = rheo::constant(rheo::SetpointAndHysteresis(20.0f, 2.0f));
  auto therm = rheo::constant(16.0f);
  auto bb = rheo::bangBang(therm, sph);
  auto lastPc = rheo::ProcessCommand::neutral;
  auto pull = bb([&lastPc](auto v) { lastPc = v; }, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(rheo::ProcessCommand::up, lastPc, "After pulling, process command should be up");
}

void test_bangBang_goes_down() {
  auto sph = rheo::constant(rheo::SetpointAndHysteresis(20.0f, 2.0f));
  auto therm = rheo::constant(24.0f);
  auto bb = rheo::bangBang(therm, sph);
  auto lastPc = rheo::ProcessCommand::neutral;
  auto pull = bb([&lastPc](auto v) { lastPc = v; }, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(rheo::ProcessCommand::down, lastPc, "After pulling, process command should be down");
}

void test_bangBang_does_nothing() {
  auto sph = rheo::constant(rheo::SetpointAndHysteresis(20.0f, 2.0f));
  auto therm = rheo::constant(20.0f);
  auto bb = rheo::bangBang(therm, sph);
  auto lastPc = rheo::ProcessCommand::neutral;
  auto pull = bb([&lastPc](auto v) { lastPc = v; }, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(rheo::ProcessCommand::neutral, lastPc, "After pulling, process command should still be neutral");
}

void test_bangBang_goes_up_then_down() {
  auto sph = rheo::constant(rheo::SetpointAndHysteresis(20.0f, 2.0f));
  auto therm = rheo::sequence(16.0f, 24.0f, 0.5f);
  auto bb = rheo::bangBang(therm, sph);
  auto lastPc = rheo::ProcessCommand::neutral;
  auto pull = bb([&lastPc](auto v) { lastPc = v; }, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(rheo::ProcessCommand::up, lastPc, "Should start by pushing process up");
  for (int i = 0; i < 3; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(rheo::ProcessCommand::up, lastPc, "Should still be going up");
  }
  for (int i = 0; i < 9; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(rheo::ProcessCommand::up, lastPc, "Should be still going up while in happy range");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(rheo::ProcessCommand::down, lastPc, "Should now be going down after hitting upper limit");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_bangBang_goes_up);
  RUN_TEST(test_bangBang_goes_down);
  RUN_TEST(test_bangBang_goes_up_then_down);
  UNITY_END();
}
