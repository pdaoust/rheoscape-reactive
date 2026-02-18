#include <unity.h>
#include <util/logging.hpp>

using namespace rheoscape::logging;

void test_logging_logs_each_level_correctly() {
  uint8_t last_log_level;
  register_subscriber([&last_log_level](uint8_t level, std::optional<std::string> topic, std::string message) {
    last_log_level = level;
  });
  trace("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_TRACE, last_log_level, "Should have logged a trace message");
  debug("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_DEBUG, last_log_level, "Should have logged a debug message");
  info("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_INFO, last_log_level, "Should have logged a info message");
  warn("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_WARN, last_log_level, "Should have logged a warn message");
  error("", "");
  TEST_ASSERT_EQUAL_MESSAGE(LOG_LEVEL_ERROR, last_log_level, "Should have logged a error message");
}

void test_logging_passes_topic() {
  std::optional<std::string> last_topic;
  register_subscriber([&last_topic](uint8_t level, std::optional<std::string> topic, std::string message) {
    last_topic = topic;
  });
  trace("hello", "");
  TEST_ASSERT_TRUE_MESSAGE(last_topic.has_value(), "Last topic should have a value");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", last_topic.value().c_str(), "Last topic should have hte right value");
}

void test_logging_passes_message() {
  std::optional<std::string> last_message;
  register_subscriber([&last_message](uint8_t level, std::optional<std::string> topic, std::string message) {
    last_message = message;
  });
  trace("", "hello");
  TEST_ASSERT_TRUE_MESSAGE(last_message.has_value(), "Last topic should have a value");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", last_message.value().c_str(), "Last topic should have hte right value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_logging_logs_each_level_correctly);
  RUN_TEST(test_logging_passes_topic);
  RUN_TEST(test_logging_passes_message);
  UNITY_END();
}