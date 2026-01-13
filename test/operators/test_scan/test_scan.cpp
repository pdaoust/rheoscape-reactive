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
  auto some_numbers = unwrap_endable(sequence(0, 9, 1));
  auto scanner = scan(
    some_numbers,
    (std::string)"",
    [](std::string acc, int v) {
      return acc + std::to_string(v);
    }
  );
  std::string pushed_value;
  auto pull = scanner([&pushed_value](std::string v) { pushed_value = v; });
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0", pushed_value.c_str(), "should scan initial acc with first item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("01", pushed_value.c_str(), "should scan acc with second item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("012", pushed_value.c_str(), "should scan acc with third item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0123", pushed_value.c_str(), "should scan acc with fourth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("01234", pushed_value.c_str(), "should scan acc with fifth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("012345", pushed_value.c_str(), "should scan acc with sixth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0123456", pushed_value.c_str(), "should scan acc with seventh item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("01234567", pushed_value.c_str(), "should scan acc with eighth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("012345678", pushed_value.c_str(), "should scan acc with ninth item");
  pull();
  TEST_ASSERT_EQUAL_STRING_MESSAGE("0123456789", pushed_value.c_str(), "should scan acc with tenth item");
}

void test_scan_scans_without_initial() {
  auto some_numbers = unwrap_endable(sequence(0, 9, 1));
  auto scanner = scan(
    some_numbers,
    [](int acc, int v) {
      return acc + v;
    }
  );
  int pushed_value = -1;
  pull_fn pull = scanner([&pushed_value](auto v) { pushed_value = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(-1, pushed_value, "should not yield first item by itself");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value, "should combine first item with second item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_value, "should combine acc with third item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(6, pushed_value, "should combine acc with fourth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(10, pushed_value, "should combine acc with fifth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(15, pushed_value, "should combine acc with sixth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(21, pushed_value, "should combine acc with seventh item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(28, pushed_value, "should combine acc with eighth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(36, pushed_value, "should combine acc with ninth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(45, pushed_value, "should combine acc with tenth item");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_scan_scans_with_initial);
  RUN_TEST(test_scan_scans_without_initial);
  UNITY_END();
}