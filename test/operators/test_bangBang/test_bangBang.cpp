#include <unity.h>
#include <operators/bangBang.hpp>
#include <sources/constant.hpp>
#include <types/State.hpp>

void test_bangBang_goes_up() {
  auto sph = rheo::constant(rheo::SetpointAndHysteresis(20.0f, 2.0f));
  auto therm = rheo::constant(16.0f);
  auto bb = rheo::bangBang(therm, sph);
  auto lastPc = rheo::ProcessCommand::neutral;
  auto pull = bb([&lastPc](auto v) { lastPc = v; }, [](){});
  TEST_ASSERT_EQUAL_MESSAGE(rheo::ProcessCommand::up, lastPc, "After pulling, process command should be up");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_bangBang_goes_up);
  UNITY_END();
}
