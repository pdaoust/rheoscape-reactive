#include <unity.h>
#include <functional>
#include <operators/scan.hpp>
#include <sources/sequence.hpp>
#include <sources/empty.hpp>

void test_scan_scans() {
  auto someNumbers = rheo::sequence(0, 9, 1);
  auto scanner = rheo::scan<std::string>(
    someNumbers,
    "",
    (rheo::fold_fn<std::string, int>)[](std::string acc, int v) {
      return acc + std::to_string(v);
    }
  );
  std::string pushedValue;
  rheo::pull_fn pull = scanner([&pushedValue](std::string v) { pushedValue = v; }, [](){});
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

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_scan_scans);
  UNITY_END();
}