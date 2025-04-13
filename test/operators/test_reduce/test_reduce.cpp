#include <unity.h>
#include <functional>
#include <operators/reduce.hpp>
#include <sources/sequence.hpp>
#include <sources/empty.hpp>

void test_reduce_reduces() {
  auto someNumbers = rheo::sequence(0, 9, 1);
  auto reducer = rheo::reduce(
    someNumbers,
    (rheo::reduce_fn<int>)[](int acc, int v) {
      return acc + v;
    }
  );
  int pushedValue;
  rheo::pull_fn pull = reducer([&pushedValue](int v) { pushedValue = v; }, [](){});
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(0, pushedValue, "should yield first item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "should reduce first item with second item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedValue, "should reduce acc with third item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(6, pushedValue, "should reduce acc with fourth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(10, pushedValue, "should reduce acc with fifth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(15, pushedValue, "should reduce acc with sixth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(21, pushedValue, "should reduce acc with seventh item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(28, pushedValue, "should reduce acc with eighth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(36, pushedValue, "should reduce acc with ninth item");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(45, pushedValue, "should reduce acc with tenth item");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_reduce_reduces);
  UNITY_END();
}