#include <unity.h>
#include <operators/tap.hpp>
#include <sources/constant.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;


void test_tap_calls_tap_function() {
  auto source = constant(5);
  int tapped_value = 0;
  auto tapped = tap(source, [&tapped_value](source_fn<int> s) { s([&tapped_value](int v) { tapped_value = v; }); });
  auto pull = tapped([](int _){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(5, tapped_value, "Should have passed pushed value to tap function");
}

void test_tap_passes_on_pushed_value() {
  auto source = constant(5);
  auto tapped = tap(source, [](source_fn<int> s){ s([](int _){}); });
  int pushed_value = 0;
  auto pull = tapped([&pushed_value](int v){ pushed_value = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(5, pushed_value, "Should have passed pushed value to downstream");
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_tap_calls_tap_function);
  RUN_TEST(test_tap_passes_on_pushed_value);
  UNITY_END();
}