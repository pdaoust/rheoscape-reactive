#include <unity.h>
#include <operators/count.hpp>
#include <operators/unwrap.hpp>
#include <sources/sequence.hpp>

using namespace rheo;
using namespace rheo::sources;
using namespace rheo::operators;

void test_count_counts() {
  auto some_numbers = sequence(1, 15, 1);
  auto counter = count(some_numbers);
  int pushed_value = 0;
  pull_fn pull = counter([&pushed_value](int v) { pushed_value = v; });
  for (int i = 1; i <= 15; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value, "counter should count");
  }
}

void test_tag_count_tags_and_counts() {
  auto some_numbers = unwrap_endable(sequence(1, 15, 1));
  auto counter = tag_count(some_numbers);
  TaggedValue<int, unsigned long> pushed_value;
  pull_fn pull = counter([&pushed_value](TaggedValue<int, unsigned long> v) { pushed_value = v; });
  for (int i = 1; i <= 15; i ++) {
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, pushed_value.tag, "counter should count");
  }
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_count_counts);
  RUN_TEST(test_tag_count_tags_and_counts);
  UNITY_END();
}