#include <unity.h>
#include <types/State.hpp>

void test_State_updates() {
  rheo::State<int> myState(11);
  TEST_ASSERT_EQUAL_MESSAGE(11, myState.get(), "Should be 11");
  myState.set(15);
  TEST_ASSERT_EQUAL_MESSAGE(15, myState.get(), "Should now be 15");
}

void test_State_ends() {
  rheo::State<int> myState;
  TEST_ASSERT_FALSE_MESSAGE(myState.isEnded(), "Shouldn't be ended yet");
  myState.end();
  TEST_ASSERT_TRUE_MESSAGE(myState.isEnded(), "Should be ended now");
}

void test_State_throws_exception_when_unset() {
  rheo::State<int> myState;
  try {
    myState.get();
    TEST_FAIL_MESSAGE("Should've thrown exception");
  } catch (rheo::bad_state_unset_access e) {
    TEST_PASS_MESSAGE(e.what());
  }
  TEST_FAIL_MESSAGE("Didn't throw the right kind of exception at all");
}

void test_State_throws_exception_when_ended() {
  rheo::State<int> myState(11);
  myState.end();
  try {
    myState.get();
    TEST_FAIL_MESSAGE("Should've thrown exception");
  } catch (rheo::bad_state_ended_access e) {
    TEST_PASS_MESSAGE(e.what());
  }
  TEST_FAIL_MESSAGE("Didn't throw the right kind of exception at all");
}

void test_State_pushes_value_when_set() {
  rheo::State<int> myState;
  int pushedValue = 0;
  myState.addSink([&pushedValue](int v) { pushedValue = v; }, [](){});
  TEST_ASSERT_EQUAL_MESSAGE(0, pushedValue, "Should be 0 before a myState is set");
  myState.set(11);
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedValue, "Should be 11 after setting");
}

void test_State_pushes_state_to_new_subscriber() {
  rheo::State<int> myState(11);
  int pushedValue = 0;
  myState.addSink([&pushedValue](int v) { pushedValue = v; }, [](){});
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedValue, "New subscriber 1 should immediately get initial myState");
  myState.set(15);
  int pushedValue2 = 0;
  myState.addSink([&pushedValue2](int v) { pushedValue2 = v; }, [](){});
  TEST_ASSERT_EQUAL_MESSAGE(15, pushedValue2, "New subscriber 2 should immediately get new myState");
}

void test_State_sends_end_signal() {
  rheo::State<int> myState;
  bool didEnd = false;
  myState.addSink([](int _){}, [&didEnd]() { didEnd = true; });
  TEST_ASSERT_FALSE_MESSAGE(didEnd, "Subscriber shouldn't get end signal yet");
  myState.end();
  TEST_ASSERT_TRUE_MESSAGE(didEnd, "Subscriber should get end signal when myState ends");
}

void test_State_can_be_pulled() {
  rheo::State<int> myState(11);
  int pushedCount = 0;
  auto pull = myState.addSink([&pushedCount](int _) { pushedCount ++; }, [](){});
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "should have been pushed on subscribe");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedCount, "should have been pushed on first pull");
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedCount, "should have been pushed on second pull");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_State_updates);
  RUN_TEST(test_State_ends);
  RUN_TEST(test_State_throws_exception_when_unset);
  RUN_TEST(test_State_throws_exception_when_ended);
  RUN_TEST(test_State_pushes_value_when_set);
  RUN_TEST(test_State_pushes_state_to_new_subscriber);
  RUN_TEST(test_State_sends_end_signal);
  RUN_TEST(test_State_can_be_pulled);
  UNITY_END();
}
