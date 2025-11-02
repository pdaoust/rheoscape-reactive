#include <unity.h>
#include <fmt/format.h>
#include <operators/foreach.hpp>
#include <sources/constant.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_foreach_is_called_for_each() {
  int pushedCount = 0;
  auto source = constant(true);
  auto pull = source | foreach([&pushedCount](bool _) { pushedCount ++; });
  for (int i = 1; i < 10; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedCount, fmt::format("Should have pushed {} times", i).c_str());
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_foreach_is_called_for_each);
  UNITY_END();
}