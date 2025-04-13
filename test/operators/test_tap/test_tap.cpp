#include <unity.h>
#include <operators/tap.hpp>
#include <sources/constant.hpp>

void test_tap_calls_tap_function() {
  auto source = rheo::constant(5);
  int tappedValue = 0;
  auto tapped = rheo::tap(source, (rheo::exec_fn<int>)[&tappedValue](int v) { tappedValue = v; });
  auto pull = tapped([](int _){}, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(5, tappedValue, "Should have passed pushed value to tap function");
}

void test_tap_passes_on_pushed_value() {
  auto source = rheo::constant(5);
  auto tapped = rheo::tap(source, (rheo::exec_fn<int>)[](int _){});
  int pushedValue = 0;
  auto pull = tapped([&pushedValue](int v){ pushedValue = v; }, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(5, pushedValue, "Should have passed pushed value to downstream");
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_tap_calls_tap_function);
  RUN_TEST(test_tap_passes_on_pushed_value);
  UNITY_END();
}