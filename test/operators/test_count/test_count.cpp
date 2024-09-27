#include <unity.h>
#include <operators/count.hpp>
#include <sources/sequence.hpp>

void test_count_counts() {
  auto someNumbers = rheo::sequence(1, 15, 1);
  auto counter = rheo::count(someNumbers);
  int pushedValue = 0;
  rheo::pull_fn pull = counter([&pushedValue](int v) { pushedValue = v; }, [](){});
  for (int i = 1; i <= 15; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushedValue, "counter should count");
  }
}

void test_tagCount_tags_and_counts() {
  auto someNumbers = rheo::sequence(1, 15, 1);
  auto counter = rheo::tagCount(someNumbers);
  rheo::TaggedValue<int, size_t> pushedValue;
  rheo::pull_fn pull = counter([&pushedValue](rheo::TaggedValue<int, size_t> v) { pushedValue = v; }, [](){});
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