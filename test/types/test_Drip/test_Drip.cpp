#include <unity.h>
#include <sources/fromIterator.hpp>
#include <types/Drip.hpp>

void test_Drip_drips() {
  std::vector<int> numbers { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  auto source = rheo::fromIterator(numbers.begin(), numbers.end());
  int lastNumberDripped;
  auto dripper = rheo::Drip<int>(
    source,
    [&lastNumberDripped](int v) { lastNumberDripped = v; },
    [](){}
  );

  for (int i = 0; i < 10; i ++) {
    dripper.drip();
    TEST_ASSERT_EQUAL_MESSAGE(i, lastNumberDripped, "Next number should've been dripped");
  }
}

void test_Drip_ends() {
  std::vector<int> numbers { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  auto source = rheo::fromIterator(numbers.begin(), numbers.end());
  int endCount = 0;
  auto dripper = rheo::Drip<int>(
    source,
    [](int _) {},
    [&endCount](){ endCount ++; }
  );

  for (int i = 0; i < 10; i ++) {
    dripper.drip();
  }

  TEST_ASSERT_EQUAL_MESSAGE(1, endCount, "end_fn should've been called after source ended");

  for (int i = 2; i < 5; i ++) {
    dripper.drip();
    TEST_ASSERT_EQUAL_MESSAGE(i, endCount, "end_fn should've been called on drip attempt for ended source");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_Drip_drips);
  RUN_TEST(test_Drip_ends);
  UNITY_END();
}
