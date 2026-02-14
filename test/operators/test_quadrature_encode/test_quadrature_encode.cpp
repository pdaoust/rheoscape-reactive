#include <unity.h>
#include <functional>
#include <operators/quadrature_encode.hpp>
#include <operators/unwrap.hpp>
#include <sources/from_iterator.hpp>

using namespace rheo;
using namespace rheo::operators;
using namespace rheo::sources;

// Helper: create an unwrapped bool source from a vector.
// The vector must outlive the source.
auto make_pin_source(std::vector<bool>& values) {
  return unwrap_endable(from_iterator(values.begin(), values.end()));
}

// Quadrature state encoding: state = (A << 1) | B.
// Initial state assumed by the operator is 3 (A=1, B=1).
//
// CW full detent from (1,1): (1,1) -> (0,1) -> (0,0) -> (1,0) -> (1,1)
//   A: 1, 0, 0, 1, 1
//   B: 1, 1, 0, 0, 1
//
// CCW full detent from (1,1): (1,1) -> (1,0) -> (0,0) -> (0,1) -> (1,1)
//   A: 1, 1, 0, 0, 1
//   B: 1, 0, 0, 1, 1

void test_one_clockwise_detent() {
  // First value (1,1) matches the initial assumed state, producing no output.
  // Next 4 transitions complete one CW detent.
  std::vector<bool> a_vals = { 1, 0, 0, 1, 1 };
  std::vector<bool> b_vals = { 1, 1, 0, 0, 1 };

  auto encoded = quadrature_encode(make_pin_source(a_vals), make_pin_source(b_vals));

  std::vector<QuadratureEncodeDirection> results;
  pull_fn pull = encoded([&results](QuadratureEncodeDirection d) {
    results.push_back(d);
  });

  // Pull through all 5 states.
  for (int i = 0; i < 5; i++) {
    pull();
  }

  TEST_ASSERT_EQUAL_MESSAGE(1, results.size(), "should produce exactly one click");
  TEST_ASSERT_EQUAL_MESSAGE(QuadratureEncodeDirection::Clockwise, results[0], "should be clockwise");
}

void test_one_counter_clockwise_detent() {
  std::vector<bool> a_vals = { 1, 1, 0, 0, 1 };
  std::vector<bool> b_vals = { 1, 0, 0, 1, 1 };

  auto encoded = quadrature_encode(make_pin_source(a_vals), make_pin_source(b_vals));

  std::vector<QuadratureEncodeDirection> results;
  pull_fn pull = encoded([&results](QuadratureEncodeDirection d) {
    results.push_back(d);
  });

  for (int i = 0; i < 5; i++) {
    pull();
  }

  TEST_ASSERT_EQUAL_MESSAGE(1, results.size(), "should produce exactly one click");
  TEST_ASSERT_EQUAL_MESSAGE(QuadratureEncodeDirection::CounterClockwise, results[0], "should be counter-clockwise");
}

void test_two_clockwise_detents() {
  // Two full CW rotations back to back.
  std::vector<bool> a_vals = { 1, 0, 0, 1, 1, 0, 0, 1, 1 };
  std::vector<bool> b_vals = { 1, 1, 0, 0, 1, 1, 0, 0, 1 };

  auto encoded = quadrature_encode(make_pin_source(a_vals), make_pin_source(b_vals));

  std::vector<QuadratureEncodeDirection> results;
  pull_fn pull = encoded([&results](QuadratureEncodeDirection d) {
    results.push_back(d);
  });

  for (int i = 0; i < 9; i++) {
    pull();
  }

  TEST_ASSERT_EQUAL_MESSAGE(2, results.size(), "should produce two clicks");
  TEST_ASSERT_EQUAL_MESSAGE(QuadratureEncodeDirection::Clockwise, results[0], "first click CW");
  TEST_ASSERT_EQUAL_MESSAGE(QuadratureEncodeDirection::Clockwise, results[1], "second click CW");
}

void test_direction_reversal() {
  // One CW detent, then one CCW detent.
  // CW:  (1,1) -> (0,1) -> (0,0) -> (1,0) -> (1,1)
  // CCW: (1,1) -> (1,0) -> (0,0) -> (0,1) -> (1,1)
  std::vector<bool> a_vals = { 1, 0, 0, 1, 1, 1, 0, 0, 1 };
  std::vector<bool> b_vals = { 1, 1, 0, 0, 1, 0, 0, 1, 1 };

  auto encoded = quadrature_encode(make_pin_source(a_vals), make_pin_source(b_vals));

  std::vector<QuadratureEncodeDirection> results;
  pull_fn pull = encoded([&results](QuadratureEncodeDirection d) {
    results.push_back(d);
  });

  for (int i = 0; i < 9; i++) {
    pull();
  }

  TEST_ASSERT_EQUAL_MESSAGE(2, results.size(), "should produce two clicks");
  TEST_ASSERT_EQUAL_MESSAGE(QuadratureEncodeDirection::Clockwise, results[0], "first click CW");
  TEST_ASSERT_EQUAL_MESSAGE(QuadratureEncodeDirection::CounterClockwise, results[1], "second click CCW");
}

void test_incomplete_turn_debounces() {
  // Start CW for 2 quarter-steps, then reverse back.
  // This simulates the user starting to turn and then reversing —
  // the quarter-click accumulator should cancel out without emitting a click.
  // CW half: (1,1) -> (0,1) -> (0,0)  (+2 quarter clicks)
  // Reverse:         (0,0) -> (0,1) -> (1,1)  (-2 quarter clicks, back to 0)
  std::vector<bool> a_vals = { 1, 0, 0, 0, 1 };
  std::vector<bool> b_vals = { 1, 1, 0, 1, 1 };

  auto encoded = quadrature_encode(make_pin_source(a_vals), make_pin_source(b_vals));

  std::vector<QuadratureEncodeDirection> results;
  pull_fn pull = encoded([&results](QuadratureEncodeDirection d) {
    results.push_back(d);
  });

  for (int i = 0; i < 5; i++) {
    pull();
  }

  TEST_ASSERT_EQUAL_MESSAGE(0, results.size(),
    "incomplete turn reversed back should produce no clicks (debounce)");
}

void test_no_change_produces_no_output() {
  // Repeated same state — no transitions.
  std::vector<bool> a_vals = { 1, 1, 1, 1 };
  std::vector<bool> b_vals = { 1, 1, 1, 1 };

  auto encoded = quadrature_encode(make_pin_source(a_vals), make_pin_source(b_vals));

  std::vector<QuadratureEncodeDirection> results;
  pull_fn pull = encoded([&results](QuadratureEncodeDirection d) {
    results.push_back(d);
  });

  for (int i = 0; i < 4; i++) {
    pull();
  }

  TEST_ASSERT_EQUAL_MESSAGE(0, results.size(), "no state changes should produce no clicks");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_one_clockwise_detent);
  RUN_TEST(test_one_counter_clockwise_detent);
  RUN_TEST(test_two_clockwise_detents);
  RUN_TEST(test_direction_reversal);
  RUN_TEST(test_incomplete_turn_debounces);
  RUN_TEST(test_no_change_produces_no_output);
  UNITY_END();
}
