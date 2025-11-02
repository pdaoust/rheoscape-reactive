#include <unity.h>
#include <operators/count.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::sources;
using namespace rheo::operators;

void test_count_counts() {
  auto someNumbers = sequence(1, 15, 1);
  auto counter = count(someNumbers);
  int pushedValue = 0;
  pull_fn pull = counter([&pushedValue](int v) { pushedValue = v; });
  for (int i = 1; i <= 15; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "counter should count");
  }
}

void test_tagCount_tags_and_counts() {
  auto someNumbers = unwrapEndable(sequence(1, 15, 1));
  auto counter = tagCount(someNumbers);
  TaggedValue<int, unsigned long> pushedValue;
  pull_fn pull = counter([&pushedValue](TaggedValue<int, unsigned long> v) { pushedValue = v; });
  for (int i = 1; i <= 15; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue.tag, "counter should count");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_count_counts);
  RUN_TEST(test_tagCount_tags_and_counts);
  UNITY_END();
}