#include <unity.h>
#include <types/core_types.hpp>
#include <util/pipes.hpp>
#include <types/mock_clock.hpp>
#include <sources/constant.hpp>
#include <sources/from_clock.hpp>
#include <operators/map.hpp>
#include <operators/filter.hpp>
#include <operators/dedupe.hpp>
#include <operators/settle.hpp>
#include <operators/throttle.hpp>
#include <operators/sample.hpp>
#include <operators/scan.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::util;
using namespace rheoscape::sources;

void setUp() {}
void tearDown() {}

// Scenario 1: map with a fully generic (auto) lambda.
// The lambda's input/output types are unknown until called,
// so typed_pipe<int, std::string> pins them.
void test_map_with_auto_lambda() {
  auto pipe = typed_pipe<int, std::string>(
    map([](auto v) { return std::to_string(v); })
  );

  // Verify the pipe satisfies Pipe concept with correct types.
  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, std::string>);

  // Actually pipe a source through it.
  auto result_source = constant(42) | pipe;
  std::string received;
  auto pull = result_source([&](std::string v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL_STRING("42", received.c_str());
}

// Scenario 2: map with a typed lambda (int -> int).
// typed_pipe just annotates it.
void test_map_with_typed_lambda_same_type() {
  auto pipe = typed_pipe<int>(
    map([](int v) { return v * 2; })
  );

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, int>);

  auto result_source = constant(5) | pipe;
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(10, received);
}

// Scenario 2b: map with a generic (auto) lambda, same in/out type.
// The single type parameter pins both input and output.
void test_map_with_auto_lambda_same_type() {
  auto pipe = typed_pipe<int>(
    map([](auto v) { return v * 2; })
  );

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, int>);

  auto result_source = constant(5) | pipe;
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(10, received);
}

// Scenario 3: filter with a generic predicate.
// Filter is same-type in/out, so typed_pipe<int> suffices.
void test_filter_with_auto_predicate() {
  auto pipe = typed_pipe<int>(
    filter([](auto v) { return v > 0; })
  );

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, int>);

  auto result_source = constant(5) | pipe;
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(5, received);
}

// Scenario 4: dedupe() takes no parameters at all.
// It's completely generic until called.
void test_dedupe_no_params() {
  auto pipe = typed_pipe<int>(dedupe());

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, int>);

  auto result_source = constant(7) | pipe;
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(7, received);
}

// Scenario 5: settle with a clock source.
// settle's pipe factory has no type aliases;
// typed_pipe pins the value type.
void test_settle_pipe() {
  mock_clock_ulong_millis::set_time(0);
  auto pipe = typed_pipe<int>(
    settle(from_clock<mock_clock_ulong_millis>(), mock_clock_ulong_millis::duration(10))
  );

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, int>);
  TEST_PASS();
}

// Scenario 6: throttle with a clock source.
void test_throttle_pipe() {
  mock_clock_ulong_millis::set_time(0);
  auto pipe = typed_pipe<int>(
    throttle(from_clock<mock_clock_ulong_millis>(), mock_clock_ulong_millis::duration(10))
  );

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, int>);
  TEST_PASS();
}

// Scenario 7: sample_every gives a fully generic pipe
// (it knows the event source but not the sample source type).
// typed_pipe pins the sample source's value type as input
// and the tuple output type.
void test_sample_every_pipe() {
  mock_clock_ulong_millis::set_time(0);
  using ClockValue = mock_clock_ulong_millis::time_point;
  auto pipe = typed_pipe<int, std::tuple<ClockValue, int>>(
    sample_every(from_clock<mock_clock_ulong_millis>())
  );

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(
    std::same_as<pipe_output_t<decltype(pipe)>, std::tuple<ClockValue, int>>
  );
  TEST_PASS();
}

// Scenario 8: scan with an initial value changes the type.
// scan<float>(0.0f, scanner) takes int input and produces float output.
void test_scan_type_changing_pipe() {
  auto pipe = typed_pipe<int, float>(
    scan(0.0f, [](float acc, int v) { return acc + static_cast<float>(v); })
  );

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, float>);

  auto result_source = constant(3) | pipe;
  float received = 0.0f;
  auto pull = result_source([&](float v) { received = v; });
  pull();
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, received);
}

// Scenario 8b: scan with fully generic (auto) accumulator and value.
// typed_pipe<int, float> pins the types;
// SFINAE ensures the scanner's return type matches the accumulator.
void test_scan_type_changing_pipe_auto_args() {
  auto pipe = typed_pipe<int, float>(
    scan(0.0f, [](auto acc, auto v) { return acc + static_cast<float>(v); })
  );

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, float>);

  auto result_source = constant(3) | pipe;
  float received = 0.0f;
  auto pull = result_source([&](float v) { received = v; });
  pull();
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, received);
}

// Scenario 9: Composing two typed_pipe wrappers with `|`.
// This tests that the output of one typed_pipe
// can be piped into another typed_pipe.
void test_compose_two_typed_pipes() {
  auto pipe1 = typed_pipe<int>(
    map([](int v) { return v + 1; })
  );
  auto pipe2 = typed_pipe<int, std::string>(
    map([](int v) { return std::to_string(v); })
  );

  auto result_source = constant(9) | pipe1 | pipe2;
  std::string received;
  auto pull = result_source([&](std::string v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL_STRING("10", received.c_str());
}

// Scenario 10: typed_pipe_wrapper is copyable
// so the same pipe can be used in multiple pipelines.
void test_typed_pipe_is_copyable() {
  auto pipe = typed_pipe<int>(
    map([](int v) { return v * 3; })
  );

  auto source_a = constant(2) | pipe;
  auto source_b = constant(4) | pipe;

  int a = 0, b = 0;
  auto pull_a = source_a([&](int v) { a = v; });
  auto pull_b = source_b([&](int v) { b = v; });
  pull_a();
  pull_b();
  TEST_ASSERT_EQUAL(6, a);
  TEST_ASSERT_EQUAL(12, b);
}

// compose_pipes: single pipe (degenerate case).
void test_compose_pipes_single() {
  auto composed = compose_pipes(
    map([](int v) { return v * 2; })
  );

  auto result_source = constant(5) | composed;
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(10, received);
}

// compose_pipes: two same-type pipes.
void test_compose_pipes_same_type() {
  auto composed = compose_pipes(
    map([](int v) { return v + 1; }),
    dedupe()
  );

  auto result_source = constant(5) | composed;
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(6, received);
}

// compose_pipes: type-changing pipes (int -> string).
void test_compose_pipes_type_changing() {
  auto composed = compose_pipes(
    map([](int v) { return v + 1; }),
    map([](int v) { return std::to_string(v); })
  );

  auto result_source = constant(9) | composed;
  std::string received;
  auto pull = result_source([&](std::string v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL_STRING("10", received.c_str());
}

// compose_pipes: multi-stage pipeline (filter + map + dedupe).
void test_compose_pipes_multi_stage() {
  auto composed = compose_pipes(
    filter([](int v) { return v > 0; }),
    map([](int v) { return v * 10; }),
    dedupe()
  );

  auto result_source = constant(3) | composed;
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(30, received);
}

// compose_pipes wrapped in typed_pipe satisfies Pipe concept.
void test_compose_pipes_with_typed_pipe() {
  auto pipe = typed_pipe<int, std::string>(
    compose_pipes(
      filter([](int v) { return v > 0; }),
      map([](int v) { return std::to_string(v); })
    )
  );

  static_assert(concepts::Pipe<decltype(pipe)>);
  static_assert(std::same_as<pipe_input_t<decltype(pipe)>, int>);
  static_assert(std::same_as<pipe_output_t<decltype(pipe)>, std::string>);

  auto result_source = constant(42) | pipe;
  std::string received;
  auto pull = result_source([&](std::string v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL_STRING("42", received.c_str());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_map_with_auto_lambda);
  RUN_TEST(test_map_with_typed_lambda_same_type);
  RUN_TEST(test_map_with_auto_lambda_same_type);
  RUN_TEST(test_filter_with_auto_predicate);
  RUN_TEST(test_dedupe_no_params);
  RUN_TEST(test_settle_pipe);
  RUN_TEST(test_throttle_pipe);
  RUN_TEST(test_sample_every_pipe);
  RUN_TEST(test_scan_type_changing_pipe);
  RUN_TEST(test_scan_type_changing_pipe_auto_args);
  RUN_TEST(test_compose_two_typed_pipes);
  RUN_TEST(test_typed_pipe_is_copyable);
  RUN_TEST(test_compose_pipes_single);
  RUN_TEST(test_compose_pipes_same_type);
  RUN_TEST(test_compose_pipes_type_changing);
  RUN_TEST(test_compose_pipes_multi_stage);
  RUN_TEST(test_compose_pipes_with_typed_pipe);
  UNITY_END();
}
