#include <unity.h>
#include <util.hpp>
#include <operators/pid.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_pid_is_stable_at_setpoint() {
  auto lettersSource = unwrapEndable(sequence<char>('a', 'c', 1));
  auto numbersSource = unwrapEndable(sequence(1, 3, 1));
  auto merged = merge(lettersSource, numbersSource);

  std::vector<char> pushedLetters;
  std::vector<int> pushedNumbers;
  auto pull = merged(
    [&pushedLetters, &pushedNumbers](auto v) {
      if (v.index() == 0) {
        pushedLetters.push_back(std::get<0>(v));
      } else {
        pushedNumbers.push_back(std::get<1>(v));
      }
    }
  );

  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedLetters.size(), "should have pulled one letter");
  TEST_ASSERT_EQUAL_MESSAGE('a', pushedLetters[0], "and the letter should be 'a'");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedNumbers.size(), "should have pulled one number");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedNumbers[0], "and the number should be 1");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedLetters.size(), "should have pulled two letters");
  TEST_ASSERT_EQUAL_MESSAGE('b', pushedLetters[1], "and the letter should be 'b'");
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedNumbers.size(), "should have pulled two numbers");
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedNumbers[1], "and the number should be 2");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedLetters.size(), "should have pulled three letters");
  TEST_ASSERT_EQUAL_MESSAGE('c', pushedLetters[2], "and the letter should be 'c'");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedNumbers.size(), "should have pulled three numbers");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedNumbers[2], "and the number should be 3");
}

void test_merge_merges_disparate_push_streams() {
  State<char> lettersSource;
  State<int> numbersSource;
  auto merged = merge(lettersSource.sourceFn(), numbersSource.sourceFn());

  std::variant<char, int> lastPushedValue;
  auto pull = merged(
    [&lastPushedValue](std::variant<char, int> v) { lastPushedValue = v; }
  );

  lettersSource.set('a');
  TEST_ASSERT_EQUAL_MESSAGE(0, lastPushedValue.index(), "should have pushed a letter");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<0>(lastPushedValue), "and it should be 'a'");
  numbersSource.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, lastPushedValue.index(), "should have pushed a number");
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<1>(lastPushedValue), "and it should be 1");
  numbersSource.set(2);
  TEST_ASSERT_EQUAL_MESSAGE(1, lastPushedValue.index(), "should have pushed another number");
  TEST_ASSERT_EQUAL_MESSAGE(2, std::get<1>(lastPushedValue), "and it should be 2");
  lettersSource.set('b');
  TEST_ASSERT_EQUAL_MESSAGE(0, lastPushedValue.index(), "should have pushed another letter");
  TEST_ASSERT_EQUAL_MESSAGE('b', std::get<0>(lastPushedValue), "and it should be 'b'");
}

void test_merge_merges_similar_streams() {
  State<int> numbersSource1;
  State<int> numbersSource2;
  auto merged = merge(numbersSource1.sourceFn(), numbersSource2.sourceFn());

  std::vector<int> pushedValues;
  auto pull = merged(
    [&pushedValues](int v) { pushedValues.push_back(v); }
  );

  numbersSource1.set(1);
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValues.size(), "blah");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValues[0], "blah 2");
  numbersSource2.set(-1);
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedValues.size(), "blah");
  TEST_ASSERT_EQUAL_MESSAGE(-1, pushedValues[1], "blah 2");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_merge_merges_disparate_pull_streams);
  RUN_TEST(test_merge_merges_disparate_push_streams);
  RUN_TEST(test_merge_merges_similar_streams);
  UNITY_END();
}
