#include <unity.h>
#include <types/core_types.hpp>
#include <util/pipes.hpp>
#include <sources/constant.hpp>
#include <operators/map.hpp>
#include <operators/filter.hpp>
#include <operators/dedupe.hpp>
#include <operators/foreach.hpp>

using namespace rheoscape;
using namespace rheoscape::operators;
using namespace rheoscape::util;
using namespace rheoscape::sources;

void setUp() {}
void tearDown() {}

// --- PipeLike concept smoke tests ---

// Named pipe factory satisfies PipeLike.
void test_pipe_factory_satisfies_pipelike() {
  auto fn = [](int v) { return v * 2; };
  static_assert(concepts::PipeLike<decltype(map(fn))>);
  TEST_PASS();
}

// Terminal sink (foreach) also satisfies PipeLike
// because PipeLike only checks invocability with a source, not the return type.
// This is fine — composing pipe | sink defers application until a source arrives.
void test_terminal_sink_is_pipelike() {
  auto fn = [](int v) { (void)v; };
  static_assert(concepts::PipeLike<decltype(foreach(fn))>);
  TEST_PASS();
}

// A plain int is neither a Source nor PipeLike.
void test_non_callable_not_pipelike() {
  static_assert(!concepts::PipeLike<int>);
  TEST_PASS();
}

// Named pipe factory: constant(5) | map(fn).
void test_named_pipe_factory() {
  auto result_source = constant(5) | map([](int v) { return v * 2; });
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(10, received);
}

// Bare lambda pipe: constant(5) | [](auto s) { return s | map(fn); }.
void test_bare_lambda_pipe() {
  auto result_source = constant(5) | [](auto s) {
    return s | map([](int v) { return v + 1; });
  };
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(6, received);
}

// Pre-composition: auto pipe = map(fn) | filter(pred); constant(5) | pipe.
void test_pre_composition() {
  auto pipe = map([](int v) { return v * 3; })
    | filter([](int v) { return v > 0; });
  auto result_source = constant(5) | pipe;
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(15, received);
}

// Long chain: map(fn1) | filter(pred) | map(fn2).
void test_long_chain() {
  auto pipe = map([](int v) { return v * 2; })
    | filter([](int v) { return v > 0; })
    | map([](int v) { return v + 100; });
  auto result_source = constant(3) | pipe;
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(106, received);
}

// Pre-composed applied: constant(9) | (map(fn) | filter(pred)).
void test_pre_composed_applied() {
  auto result_source = constant(9) | (
    map([](int v) { return v + 1; })
    | filter([](int v) { return v > 0; })
  );
  int received = 0;
  auto pull = result_source([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(10, received);
}

// Terminal sink: constant(5) | map(fn) | foreach(exec).
void test_terminal_sink() {
  int received = 0;
  auto pull = constant(5)
    | map([](int v) { return v * 4; })
    | foreach([&](int v) { received = v; });
  pull();
  TEST_ASSERT_EQUAL(20, received);
}

// Pipe is copyable and reusable in multiple pipelines.
void test_pipe_is_copyable() {
  auto pipe = map([](int v) { return v * 3; });

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

// --- as_source<T> tests ---

// as_source wraps a lambda into a Source without type erasure.
void test_as_source_satisfies_concept() {
  auto src = as_source<int>([](auto push) {
    push(42);
    return pull_fn([](){});
  });

  static_assert(concepts::Source<decltype(src)>);
  static_assert(std::same_as<source_value_t<decltype(src)>, int>);

  int received = 0;
  auto pull = src([&](int v) { received = v; });
  TEST_ASSERT_EQUAL(42, received);
}

// as_source works with | pipe operator.
void test_as_source_with_pipe() {
  auto src = as_source<int>([](auto push) {
    push(7);
    return pull_fn([](){});
  });

  auto piped = src | map([](int v) { return v * 3; });
  int received = 0;
  auto pull = piped([&](int v) { received = v; });
  TEST_ASSERT_EQUAL(21, received);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_pipe_factory_satisfies_pipelike);
  RUN_TEST(test_terminal_sink_is_pipelike);
  RUN_TEST(test_non_callable_not_pipelike);
  RUN_TEST(test_named_pipe_factory);
  RUN_TEST(test_bare_lambda_pipe);
  RUN_TEST(test_pre_composition);
  RUN_TEST(test_long_chain);
  RUN_TEST(test_pre_composed_applied);
  RUN_TEST(test_terminal_sink);
  RUN_TEST(test_pipe_is_copyable);
  RUN_TEST(test_as_source_satisfies_concept);
  RUN_TEST(test_as_source_with_pipe);
  UNITY_END();
}
