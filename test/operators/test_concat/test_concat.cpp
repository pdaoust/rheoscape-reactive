#include <unity.h>
#include <operators/concat.hpp>
#include <operators/unwrap.hpp>
#include <sources/from_iterator.hpp>

using namespace rheoscape;
using namespace rheoscape::sources;
using namespace rheoscape::operators;

void test_concat_concats() {
  std::vector<int> first { 1, 2, 3 };
  auto first_stream = from_iterator(first.begin(), first.end());
  std::vector<int> second { 4, 5, 6 };
  auto second_stream = from_iterator(second.begin(), second.end());
  auto concatted = concat(first_stream, second_stream);
  int last_pushed_value = 0;
  bool is_ended = false;
  pull_fn pull = concatted([&last_pushed_value, &is_ended](Endable<int> v) {
    if (v.has_value()) {
      last_pushed_value = v.value();
    } else {
      is_ended = true;
    }
 });
  for (int i = 1; i <= 6; i ++) {
    last_pushed_value = 0;
    pull();
    TEST_ASSERT_EQUAL_MESSAGE(i, last_pushed_value, "should get numbers 1 through 6");
  }
  pull();
  TEST_ASSERT_TRUE_MESSAGE(is_ended, "should be ended after chewing through both streams");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_concat_concats);
  UNITY_END();
}
