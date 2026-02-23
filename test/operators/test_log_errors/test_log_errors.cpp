#include <unity.h>
#include <iostream>
#include <util/logging.hpp>
#include <operators/log_errors.hpp>
#include <states/MemoryState.hpp>
#include <types/Fallible.hpp>
#include <fmt/format.h>

using namespace rheoscape;
using namespace rheoscape::operators;

void test_log_errors_logs_errors() {
  std::cout << "Starting\n";
  std::string error_message = "boop";
  int logged_count = 0;
  logging::register_subscriber([&error_message, &logged_count](uint8_t level, std::optional<std::string> topic, std::string message) {
    error_message = message;
    logged_count ++;
  });
  std::cout << "Registered logging subscriber\n";

  auto source = MemoryState(Fallible<int, bool>(true), false);
  auto logged_source = log_errors(source.get_source_fn(false), [](bool e) { return fmt::format("The error value was {}", e); });
  std::cout << "Built logigng pipeline\n";

  Fallible<int, bool> pushed_value(0);
  int pushed_count = 0;
  auto pull = logged_source([&pushed_value, &pushed_count](Fallible<int, bool> v) {
    pushed_value = v;
    pushed_count ++;
  });
  std::cout << "Bound to pipe\n";

  pull();
  std::cout << "Pulled\n";
  TEST_ASSERT_EQUAL_MESSAGE(1, logged_count, "Should have logged 1 error");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_count, "The error value should have been pushed");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("The error value was true", error_message.c_str(), "Error should've been logged");
  TEST_ASSERT_TRUE_MESSAGE(pushed_value.is_error(), "Error value should've been passed through");
  TEST_ASSERT_TRUE_MESSAGE(pushed_value.error(), "Error value should be correct");

  source.set(Fallible<int, bool>(3), true);
  std::cout << "Set new value\n";
  TEST_ASSERT_EQUAL_MESSAGE(1, logged_count, "No new errors should be logged");
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_count, "The non-error value should've been pushed");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("The error value was true", error_message.c_str(), "No new errors should've been pushed");
  TEST_ASSERT_FALSE_MESSAGE(pushed_value.is_error(), "The pushed value shouldn't be an error");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushed_value.value(), "The pushed value should be 3");
  std::cout << "Finished test\n";
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_log_errors_logs_errors);
  UNITY_END();
}