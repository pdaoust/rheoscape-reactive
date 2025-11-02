#include <unity.h>
#include <functional>
#include <operators/scan.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>
#include <sources/empty.hpp>

using namespace rheo;
using namespace operators;
using namespace sources;

void test_scan_scans_with_initial() {
  auto someNumbers = unwrapEndable(sequence(0, 9, 1));
  auto scanner = scan(
    someNumbers,
    (std::string)"",
    [](std::string acc, int v) {
      return acc + std::to_string(v);
    }
  );
  std::string pushedValue;
  auto pull = scanner([&pushedValue](std::string v) { pushedValue = v; });
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0", pushedValue.c_str(), "should scan initial acc with first item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("01", pushedValue.c_str(), "should scan acc with second item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("012", pushedValue.c_str(), "should scan acc with third item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0123", pushedValue.c_str(), "should scan acc with fourth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("01234", pushedValue.c_str(), "should scan acc with fifth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("012345", pushedValue.c_str(), "should scan acc with sixth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0123456", pushedValue.c_str(), "should scan acc with seventh item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("01234567", pushedValue.c_str(), "should scan acc with eighth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("012345678", pushedValue.c_str(), "should scan acc with ninth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0123456789", pushedValue.c_str(), "should scan acc with tenth item");
}

void test_scan_scans_without_initial() {
  auto someNumbers = unwrapEndable(sequence(0, 9, 1));
  auto scanner = scan(
    someNumbers,
    [](int acc, int v) {
      return acc + v;
    }
  );
  int pushedValue = -1;
  pull_fn pull = scanner([&pushedValue](auto v) { pushedValue = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(-1, pushedValue, "should not yield first item by itself");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "should combine first item with second item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedValue, "should combine acc with third item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(6, pushedValue, "should combine acc with fourth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(10, pushedValue, "should combine acc with fifth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(15, pushedValue, "should combine acc with sixth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(21, pushedValue, "should combine acc with seventh item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(28, pushedValue, "should combine acc with eighth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(36, pushedValue, "should combine acc with ninth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(45, pushedValue, "should combine acc with tenth item");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_scan_scans_with_initial);
  RUN_TEST(test_scan_scans_without_initial);
  UNITY_END();
}