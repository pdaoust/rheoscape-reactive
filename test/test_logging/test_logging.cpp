#include <unity.h>
#include <logging.hpp>

using namespace rheo::logging;

void test_logging_logs_each_level_correctly() {
  uint8_t lastLogLevel;
  registerSubscriber([&lastLogLevel](uint8_t level, std::optional<std::string> topic, std::string message) {
    lastLogLevel = level;
  });
  trace("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_TRACE, lastLogLevel, "Should have logged a trace message");
  debug("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_DEBUG, lastLogLevel, "Should have logged a debug message");
  info("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_INFO, lastLogLevel, "Should have logged a info message");
  warn("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_WARN, lastLogLevel, "Should have logged a warn message");
  error("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_ERROR, lastLogLevel, "Should have logged a error message");
}

void test_logging_passes_topic() {
  std::optional<std::string> lastTopic;
  registerSubscriber([&lastTopic](uint8_t level, std::optional<std::string> topic, std::string message) {
    lastTopic = topic;
  });
  trace("hello", "");
  TEST_ASSERT_TRUE_MESSAGE(lastTopic.has_value(), "Last topic should have a value");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", lastTopic.value().c_str(), "Last topic should have hte right value");
}

void test_logging_passes_message() {
  std::optional<std::string> lastMessage;
  registerSubscriber([&lastMessage](uint8_t level, std::optional<std::string> topic, std::string message) {
    lastMessage = message;
  });
  trace("", "hello");
  TEST_ASSERT_TRUE_MESSAGE(lastMessage.has_value(), "Last topic should have a value");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", lastMessage.value().c_str(), "Last topic should have hte right value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_logging_logs_each_level_correctly);
  RUN_TEST(test_logging_passes_topic);
  RUN_TEST(test_logging_passes_message);
  UNITY_END();
}