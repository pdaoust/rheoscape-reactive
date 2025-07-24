#include <unity.h>
#include <functional>
#include <operators/scan.hpp>
#include <sources/sequence.hpp>
#include <sources/empty.hpp>

void test_scan_scans() {
  auto someNumbers = rheo::sequence(0, 9, 1);
  auto scanr = rheo::scan(
    someNumbers,
    (rheo::scan_fn<int>)[](int acc, int v) {
      return acc + v;
    }
  );
  int pushedValue;
  rheo::pull_fn pull = scanr([&pushedValue](int v) { pushedValue = v; }, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, pushedValue, "should yield first item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "should scan first item with second item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedValue, "should scan acc with third item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(6, pushedValue, "should scan acc with fourth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(10, pushedValue, "should scan acc with fifth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(15, pushedValue, "should scan acc with sixth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(21, pushedValue, "should scan acc with seventh item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(28, pushedValue, "should scan acc with eighth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(36, pushedValue, "should scan acc with ninth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(45, pushedValue, "should scan acc with tenth item");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_scan_scans);
  UNITY_END();
}