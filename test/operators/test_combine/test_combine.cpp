#include <unity.h>
#include <util.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>
#include <sources/sequence.hpp>
#include <types/State.hpp>
#include <types/EventSource.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

void test_combine_combines_to_tuple() {
  source_fn<int> a = constant(3);
  source_fn<std::string> b = constant(std::string("hello"));
  auto a_b = combine(std::make_tuple<int, std::string>, a, b);
  std::tuple<int, std::string> pushed_value;
  pull_fn pull = a_b(
    [&pushed_value](auto v) {
      pushed_value = v;
    }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(3, std::get<0>(pushed_value), "should push a tuple and its first value should be the a value");
  TEST_ASSERT_TRUE_MESSAGE(std::get<1>(pushed_value) == "hello", "tuple's second value should be the b value");
}

void test_combine_combines_to_custom_value() {
  source_fn<int> a = constant(3);
  source_fn<int> b = constant(5);
  auto a_b = combine([](int v_a, int v_b) { return v_a + v_b; }, a, b);
  int pushed_value;
  pull_fn pull = a_b(
    [&pushed_value](auto v) {
      pushed_value = v;
    }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(8, pushed_value, "should combine two ints to sum of ints using custom combiner");
}

void test_combine_combines_on_push_from_push_source() {
  State<int> state(0);
  auto push_source = state.get_source_fn();
  auto normal_source = constant(3);
  auto combined = combine(std::make_tuple<int, int>, push_source, normal_source);
  int pushed_value_left = -1;
  int pushed_value_right = -1;
  int pushed_count = 0;
  combined(
    [&pushed_value_left, &pushed_value_right, &pushed_count](std::tuple<int, int> v) {
      pushed_value_left = std::get<0>(v);
      pushed_value_right = std::get<1>(v);
      pushed_count ++;
    }
  );
  state.set(5);
  TEST_ASSERT_EQUAL_MESSAGE(5, pushed_value_left, "Should have pushed a value from the left source");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_value_right, "Should have pushed a value from the right source");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "Should have pushed on upstream push");
}

void test_combine3_combines() {
  auto a = constant(1);
  auto b = constant('a');
  auto c = constant(true);
  auto combined = combine(std::make_tuple<int, char, bool>, a, b, c);
  auto pushed_value = std::make_tuple<int, char, bool>(-1, 'z', false);
  auto pull = combined(
    [&pushed_value](std::tuple<int, char, bool> v) { pushed_value = v; }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value), "Value 1 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value), "Value 2 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE(true, std::get<2>(pushed_value), "Value 3 should be in order");
}

void test_combine4_combines() {
  auto a = constant(1);
  auto b = constant('a');
  auto c = constant(true);
  auto d = constant(3.14f);
  auto combined = combine(std::make_tuple<int, char, bool, float>, a, b, c, d);
  auto pushed_value = std::make_tuple<int, char, bool, float>(-1, 'z', false, 9.999f);
  auto pull = combined(
    [&pushed_value](std::tuple<int, char, bool, float> v) { pushed_value = v; }
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value), "Value 1 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value), "Value 2 should be in order");
  TEST_ASSERT_EQUAL_MESSAGE(true, std::get<2>(pushed_value), "Value 3 should be in order");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(3.14f, std::get<3>(pushed_value), "Value 4 should be in order");
}

void test_combine_does_not_emit_with_stale_values_from_incomplete_cascade() {
  // This test verifies that when a source with a no-op pull (like EventSource)
  // is combined with a source that pushes spontaneously (like State),
  // the combine operator doesn't hold onto stale values from incomplete cascades.
  //
  // Scenario:
  // 1. State pushes spontaneously (simulating State.set() from elsewhere)
  //    - Combine should NOT emit because EventSource doesn't respond to pulls
  //    - The State value should NOT be stored as a stale value
  // 2. State is updated again
  // 3. EventSource pushes an event
  //    - Combine should emit with the CURRENT State value, not a stale one

  EventSource<int> event_source;
  State<std::string> state("initial");

  auto combined = combine(
    [](int event, std::string state_val) {
      return std::make_pair(event, state_val);
    },
    event_source.get_source_fn(),
    state.get_source_fn()
  );

  int emit_count = 0;
  int last_event = -1;
  std::string last_state_val = "";

  combined([&](std::pair<int, std::string> v) {
    emit_count++;
    last_event = v.first;
    last_state_val = v.second;
  });

  // Step 1: State pushes spontaneously (simulating State.set() from another pipeline)
  // This should NOT cause an emission because EventSource has no-op pull
  state.set("stale_value");
  TEST_ASSERT_EQUAL_MESSAGE(0, emit_count, "Should not emit when State pushes but EventSource can't be pulled");

  // Step 2: Update State again
  state.set("current_value");
  TEST_ASSERT_EQUAL_MESSAGE(0, emit_count, "Should still not emit - EventSource still has no value");

  // Step 3: EventSource pushes - NOW combine should emit with fresh values
  event_source.push(42);
  TEST_ASSERT_EQUAL_MESSAGE(1, emit_count, "Should emit when EventSource pushes and State can be pulled");
  TEST_ASSERT_EQUAL_MESSAGE(42, last_event, "Should have the event value");
  TEST_ASSERT_TRUE_MESSAGE(last_state_val == "current_value",
    "Should have the CURRENT State value, not a stale one from earlier pushes");
}

void test_combine_clears_stale_values_on_new_cascade() {
  // This test verifies that when a new cascade starts, any stale values
  // from a previous incomplete cascade are cleared.
  //
  // Scenario with two States (both respond to pulls):
  // 1. State A pushes, pulls State B, emits (A1, B1)
  // 2. State B pushes spontaneously with B2
  //    - Should NOT emit with stale A1 value
  //    - Should start fresh cascade, pull A, emit (A_current, B2)

  State<int> state_a(1);
  State<std::string> state_b("one");

  auto combined = combine(
    [](int a, std::string b) { return std::make_pair(a, b); },
    state_a.get_source_fn(),
    state_b.get_source_fn()
  );

  int emit_count = 0;
  int last_a = -1;
  std::string last_b = "";

  combined([&](std::pair<int, std::string> v) {
    emit_count++;
    last_a = v.first;
    last_b = v.second;
  });

  // Initial state: both States do initial_push during bind.
  // State A pushes 1, cascade pulls State B, B pushes "one", emit (1, "one")
  // Then State B's initial push comes... it should start a fresh cascade.
  // Actually, the order depends on bind order. Let's just verify final state.

  // After binding, we expect some emissions from initial pushes.
  // Reset counters to test subsequent behavior.
  emit_count = 0;
  last_a = -1;
  last_b = "";

  // Update State A - should emit with current B value
  state_a.set(10);
  TEST_ASSERT_EQUAL_MESSAGE(1, emit_count, "Should emit when State A pushes");
  TEST_ASSERT_EQUAL_MESSAGE(10, last_a, "Should have new A value");
  TEST_ASSERT_TRUE_MESSAGE(last_b == "one", "Should have pulled current B value");

  // Update State B - should emit with current A value (10, not 1)
  state_b.set("ten");
  TEST_ASSERT_EQUAL_MESSAGE(2, emit_count, "Should emit when State B pushes");
  TEST_ASSERT_EQUAL_MESSAGE(10, last_a, "Should have pulled current A value, not stale");
  TEST_ASSERT_TRUE_MESSAGE(last_b == "ten", "Should have new B value");

  // Update A again, then B - verify no stale values accumulate
  state_a.set(20);
  TEST_ASSERT_EQUAL_MESSAGE(3, emit_count, "Should emit on A push");
  TEST_ASSERT_EQUAL_MESSAGE(20, last_a, "New A value");
  TEST_ASSERT_TRUE_MESSAGE(last_b == "ten", "Current B value");

  state_b.set("twenty");
  TEST_ASSERT_EQUAL_MESSAGE(4, emit_count, "Should emit on B push");
  TEST_ASSERT_EQUAL_MESSAGE(20, last_a, "Current A value, not stale");
  TEST_ASSERT_TRUE_MESSAGE(last_b == "twenty", "New B value");
}

void test_combine_only_pulls_once_for_each_push() {
  // Two-operand combine
  auto a = sequence_open(1, 1);
  auto b = sequence_open('a', (char)1);
  auto combined2 = combine(std::make_tuple<int, char>, a, b);
  auto pushed_value2 = std::make_tuple<int, char>(-1, 'z');
  int push_count2 = 0;
  auto pull2 = combined2([&pushed_value2, &push_count2](std::tuple<int, char> v) {
    pushed_value2 = v;
    push_count2 ++;
  });
  pull2();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value2), "Should have combined the first value from source A");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value2), "Should have combined the first value from source B");
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count2, "Should only have pushed one value for each pull");

  // Three-operand combine
  auto c = sequence_open(2.0f, 0.5f);
  auto combined3 = combine(std::make_tuple<int, char, float>, a, b, c);
  auto pushed_value3 = std::make_tuple<int, char, float>(-1, 'z', 9.999f);
  int push_count3 = 0;
  auto pull3 = combined3([&pushed_value3, &push_count3](std::tuple<int, char, float> v) {
    pushed_value3 = v;
    push_count3 ++;
  });
  pull3();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value3), "Should have combined the first value from source A");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value3), "Should have combined the first value from source B");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(2.0f, std::get<2>(pushed_value3), "Should have combined the first value from source C");
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count3, "Should only have pushed one value for each pull");

  // Four-operand combine
  auto d = sequence_open<uint16_t>(3, 1);
  auto combined4 = combine(std::make_tuple<int, char, float, uint16_t>, a, b, c, d);
  auto pushed_value4 = std::make_tuple<int, char, float, int>(-1, 'z', 9.999f, 1000);
  int push_count4 = 0;
  auto pull4 = combined4([&pushed_value4, &push_count4](std::tuple<int, char, float, uint16_t> v) {
    pushed_value4 = v;
    push_count4 ++;
  });
  pull4();
  TEST_ASSERT_EQUAL_MESSAGE(1, std::get<0>(pushed_value4), "Should have combined the first value from source A");
  TEST_ASSERT_EQUAL_MESSAGE('a', std::get<1>(pushed_value4), "Should have combined the first value from source B");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(2.0f, std::get<2>(pushed_value4), "Should have combined the first value from source C");
  TEST_ASSERT_EQUAL_MESSAGE(3, std::get<3>(pushed_value4), "Should have combined the first value from source D");
  TEST_ASSERT_EQUAL_MESSAGE(1, push_count4, "Should only have pushed one value for each pull");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_combine_combines_to_tuple);
  RUN_TEST(test_combine_combines_to_custom_value);
  RUN_TEST(test_combine_combines_on_push_from_push_source);
  RUN_TEST(test_combine3_combines);
  RUN_TEST(test_combine4_combines);
  RUN_TEST(test_combine_only_pulls_once_for_each_push);
  RUN_TEST(test_combine_does_not_emit_with_stale_values_from_incomplete_cascade);
  RUN_TEST(test_combine_clears_stale_values_on_new_cascade);
  UNITY_END();
}
