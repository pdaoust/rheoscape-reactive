#include <unity.h>
#include <types/State.hpp>

void test__state_updates() {
  rheo::State<int> my_state(11);
  TEST_ASSERT_EQUAL_MESSAGE(11, my_state.get(), "Should be 11");
  my_state.set(15);
  TEST_ASSERT_EQUAL_MESSAGE(15, my_state.get(), "Should now be 15");
}

void test__state_throws_exception_when_unset() {
  rheo::State<int> my_state;
  try {
    my_state.get();
    TEST_FAIL_MESSAGE("Should've thrown exception");
  } catch (rheo::bad_state_unset_access e) {
    TEST_PASS_MESSAGE(e.what());
  }
  TEST_FAIL_MESSAGE("Didn't throw the right kind of exception at all");
}

void test__state_pushes_value_when_set() {
  rheo::State<int> my_state;
  int pushed_value = 0;
  my_state.add_sink([&pushed_value](int v) { pushed_value = v; }, [](){});
  TEST_ASSERT_EQUAL_MESSAGE(0, pushed_value, "Should be 0 before a my_state is set");
  my_state.set(11);
  TEST_ASSERT_EQUAL_MESSAGE(11, pushed_value, "Should be 11 after setting");
}

void test__state_pushes_state_to_new_subscriber() {
  rheo::State<int> my_state(11);
  int pushed_value = 0;
  my_state.add_sink([&pushed_value](int v) { pushed_value = v; }, [](){});
  TEST_ASSERT_EQUAL_MESSAGE(11, pushed_value, "New subscriber 1 should immediately get initial my_state");
  my_state.set(15);
  int pushed_value2 = 0;
  my_state.add_sink([&pushed_value2](int v) { pushed_value2 = v; }, [](){});
  TEST_ASSERT_EQUAL_MESSAGE(15, pushed_value2, "New subscriber 2 should immediately get new my_state");
}

void test__state_can_be_pulled() {
  rheo::State<int> my_state(11);
  int pushed_count = 0;
  auto pull = my_state.add_sink([&pushed_count](int _) { pushed_count ++; }, [](){});
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "should have been pushed on subscribe");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_count, "should have been pushed on first pull");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_count, "should have been pushed on second pull");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test__state_updates);
  RUN_TEST(test__state_throws_exception_when_unset);
  RUN_TEST(test__state_pushes_value_when_set);
  RUN_TEST(test__state_pushes_state_to_new_subscriber);
  RUN_TEST(test__state_can_be_pulled);
  UNITY_END();
}
