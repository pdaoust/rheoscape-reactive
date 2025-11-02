#include <unity.h>
#include <iostream>
#include <logging.hpp>
#include <operators/logErrors.hpp>
#include <types/State.hpp>
#include <Fallible.hpp>
#include <fmt/format.h>

using namespace rheo;
using namespace rheo::operators;

void test_logErrors_logs_errors() {
  std::cout << "Starting\n";
  std::string errorMessage = "boop";
  int loggedCount = 0;
  logging::registerSubscriber([&errorMessage, &loggedCount](uint8_t level, std::optional<std::string> topic, std::string message) {
    errorMessage = message;
    loggedCount ++;
  });
  std::cout << "Registered logging subscriber\n";

  auto source = State(Fallible<int, bool>(true), false);
  auto loggedSource = logErrors(source.sourceFn(false), [](bool e) { return fmt::format("The error value was {}", e); });
  std::cout << "Built logigng pipeline\n";

  Fallible<int, bool> pushedValue(0);
  int pushedCount = 0;
  auto pull = loggedSource([&pushedValue, &pushedCount](Fallible<int, bool> v) {
    pushedValue = v;
    pushedCount ++;
  });
  std::cout << "Bound to pipe\n";

  pull();
  std::cout << "Pulled\n";
  TEST_ASSERT_EQUAL_MESSAGE(1, loggedCount, "Should have logged 1 error");
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedCount, "The error value should have been pushed");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("The error value was true", errorMessage.c_str(), "Error should've been logged");
  TEST_ASSERT_TRUE_MESSAGE(pushedValue.isError(), "Error value should've been passed through");
  TEST_ASSERT_TRUE_MESSAGE(pushedValue.error(), "Error value should be correct");

  source.set(Fallible<int, bool>(3), true);
  std::cout << "Set new value\n";
  TEST_ASSERT_EQUAL_MESSAGE(1, loggedCount, "No new errors should be logged");
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedCount, "The non-error value should've been pushed");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("The error value was true", errorMessage.c_str(), "No new errors should've been pushed");
  TEST_ASSERT_FALSE_MESSAGE(pushedValue.isError(), "The pushed value shouldn't be an error");
  TEST_ASSERT_EQUAL_MESSAGE(3, pushedValue.value(), "The pushed value should be 3");
  std::cout << "Finished test\n";
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_logErrors_logs_errors);
  UNITY_END();
}