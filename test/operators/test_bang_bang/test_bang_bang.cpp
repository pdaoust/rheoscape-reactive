#include <unity.h>
#include <operators/bang_bang.hpp>
#include <operators/unwrap.hpp>
#include <sources/constant.hpp>
#include <sources/sequence.hpp>
#include <types/State.hpp>

using namespace rheoscape;
using namespace rheoscape::sources;
using namespace rheoscape::operators;

void test_bang_bang_goes_up() {
  auto sph = constant(Range(18.0f, 22.0f));
  auto therm = constant(16.0f);
  auto bb = bang_bang(therm, sph);
  auto last_pc = ProcessCommand::neutral;
  auto pull = bb([&last_pc](auto v) { last_pc = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(ProcessCommand::up, last_pc, "After pulling, process command should be up");
}

void test_bang_bang_goes_down() {
  auto sph = constant(Range(18.0f, 22.0f));
  auto therm = constant(24.0f);
  auto bb = bang_bang(therm, sph);
  auto last_pc = ProcessCommand::neutral;
  auto pull = bb([&last_pc](auto v) { last_pc = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(ProcessCommand::down, last_pc, "After pulling, process command should be down");
}

void test_bang_bang_does_nothing() {
  auto sph = constant(Range(18.0f, 22.0f));
  auto therm = constant(20.0f);
  auto bb = bang_bang(therm, sph);
  auto last_pc = ProcessCommand::neutral;
  auto pull = bb([&last_pc](auto v) { last_pc = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(ProcessCommand::neutral, last_pc, "After pulling, process command should still be neutral");
}

void test_bang_bang_goes_up_then_down() {
  auto sph = constant(Range(18.0f, 22.0f));
  auto therm = unwrap_endable(sequence(16.0f, 24.0f, 0.5f));
  auto bb = bang_bang(therm, sph);
  auto last_pc = ProcessCommand::neutral;
  auto pull = bb([&last_pc](auto v) { last_pc = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(ProcessCommand::up, last_pc, "Should start by pushing process up");
  for (int i = 0; i < 3; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(ProcessCommand::up, last_pc, "Should still be going up");
  }
  for (int i = 0; i < 9; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(ProcessCommand::up, last_pc, "Should be still going up while in happy range");
  }
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(ProcessCommand::down, last_pc, "Should now be going down after hitting upper limit");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_bang_bang_goes_up);
  RUN_TEST(test_bang_bang_goes_down);
  RUN_TEST(test_bang_bang_goes_up_then_down);
  UNITY_END();
}
