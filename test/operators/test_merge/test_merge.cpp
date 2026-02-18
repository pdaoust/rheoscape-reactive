#include <unity.h>
#include <util/misc.hpp>
#include <operators/merge.hpp>
#include <operators/unwrap.hpp>
#include <sources/constant.hpp>
#include <sources/from_iterator.hpp>
#include <sources/sequence.hpp>
#include <types/State.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::sources;

void test_merge_merges_disparate_pull_streams() {
  auto letters_source = unwrap_endable(sequence<char>('a', 'c', 1));
  auto numbers_source = unwrap_endable(sequence(1, 3, 1));
  auto merged = merge_mixed(letters_source, numbers_source);

  std::vector<char> pushed_letters;
  std::vector<int> pushed_numbers;
  auto pull = merged(
    [&pushed_letters, &pushed_numbers](auto v) {
      if (v.index() == 0) {
        pushed_letters.push_back(std::get<0>(v));
      } else {
        pushed_numbers.push_back(std::get<1>(v));
      }
    }
  );

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_letters.size(), "should have pulled one letter");
  TEST_ASSERT_EQUAL_MESSAGE('a', pushed_letters[0], "and the letter should be 'a'");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_numbers.size(), "should have pulled one number");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_numbers[0], "and the number should be 1");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_letters.size(), "should have pulled two letters");
  TEST_ASSERT_EQUAL_MESSAGE('b', pushed_letters[1], "and the letter should be 'b'");
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_numbers.size(), "should have pulled two numbers");
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_numbers[1], "and the number should be 2");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_letters.size(), "should have pulled three letters");
  TEST_ASSERT_EQUAL_MESSAGE('c', pushed_letters[2], "and the letter should be 'c'");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_numbers.size(), "should have pulled three numbers");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_numbers[2], "and the number should be 3");
}

void test_merge_merges_disparate_push_streams() {
  State<char> letters_source;
  State<int> numbers_source;
  auto merged = merge_mixed(letters_source.get_source_fn(), numbers_source.get_source_fn());

  std::variant<char, int> last_pushed_value;
  auto pull = merged(
    [&last_pushed_value](std::variant<char, int> v) { last_pushed_value = v; }
  );

  letters_source.set('a');
  TEST_ASSERT_EQUAL_MESSAGE(0, last_pushed_value.index(), "should have pushed a letter");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<0>(last_pushed_value), "and it should be 'a'");
  numbers_source.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, last_pushed_value.index(), "should have pushed a number");
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<1>(last_pushed_value), "and it should be 1");
  numbers_source.set(2);
  TEST_ASSERT_EQUAL_MESSAGE(1, last_pushed_value.index(), "should have pushed another number");
  TEST_ASSERT_EQUAL_MESSAGE(2, std::get<1>(last_pushed_value), "and it should be 2");
  letters_source.set('b');
  TEST_ASSERT_EQUAL_MESSAGE(0, last_pushed_value.index(), "should have pushed another letter");
  TEST_ASSERT_EQUAL_MESSAGE('b', std::get<0>(last_pushed_value), "and it should be 'b'");
}

void test_merge_merges_similar_streams() {
  State<int> numbers_source1;
  State<int> numbers_source2;
  auto merged = merge(numbers_source1.get_source_fn(), numbers_source2.get_source_fn());

  std::vector<int> pushed_values;
  auto pull = merged(
    [&pushed_values](int v) { pushed_values.push_back(v); }
  );

  numbers_source1.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_values.size(), "blah");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_values[0], "blah 2");
  numbers_source2.set(-1);
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_values.size(), "blah");
  TEST_ASSERT_EQUAL_MESSAGE(-1, pushed_values[1], "blah 2");
}


int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_merge_merges_disparate_pull_streams);
  RUN_TEST(test_merge_merges_disparate_push_streams);
  RUN_TEST(test_merge_merges_similar_streams);
  UNITY_END();
}
