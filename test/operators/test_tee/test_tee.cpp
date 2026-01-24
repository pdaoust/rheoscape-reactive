#include <unity.h>
#include <operators/tee.hpp>
#include <sources/constant.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;


void test_tee_calls_tee_function() {
  auto source = constant(5);
  int teed_value = 0;
  auto teed = tee(source, [&teed_value](source_fn<int> s) { s([&teed_value](int v) { teed_value = v; }); });
  auto pull = teed([](int _){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(5, teed_value, "Should have passed pushed value to tee function");
}

void test_tee_passes_on_pushed_value() {
  auto source = constant(5);
  auto teed = tee(source, [](source_fn<int> s){ s([](int _){}); });
  int pushed_value = 0;
  auto pull = teed([&pushed_value](int v){ pushed_value = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(5, pushed_value, "Should have passed pushed value to downstream");
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_tee_calls_tee_function);
  RUN_TEST(test_tee_passes_on_pushed_value);
  UNITY_END();
}
