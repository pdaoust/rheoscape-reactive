#pragma once

#include <functional>
#include <util.hpp>
#ifdef PLATFORM_ESP32
#include <Arduino.h>
#endif

namespace rheo::logging {

  const uint8_t LOG_LEVEL_SILENT = 0;
  const uint8_t LOG_LEVEL_ERROR = 1;
  const uint8_t LOG_LEVEL_WARN = 2;
  const uint8_t LOG_LEVEL_INFO = 3;
  const uint8_t LOG_LEVEL_DEBUG = 4;
  const uint8_t LOG_LEVEL_TRACE = 5;
  #define LOG_LEVEL_LABEL(l) (l == rheo::logging::LOG_LEVEL_ERROR ? "ERROR" : l == rheo::logging::LOG_LEVEL_WARN ? "WARN" : l == rheo::logging::LOG_LEVEL_INFO ? "INFO" : l == rheo::logging::LOG_LEVEL_DEBUG ? "DEBUG" : l == rheo::logging::LOG_LEVEL_TRACE ? "TRACE" : "")

  namespace {
    static std::vector<std::function<void(uint8_t, std::optional<std::string>, std::string)>> _logSubscribers;
  }

  void registerSubscriber(std::function<void(uint8_t, std::optional<std::string>, std::string)> subscriber) {
    _logSubscribers.push_back(subscriber);
  }

  void log(uint8_t level, std::optional<std::string> topic, std::string message) {
    for (auto subscriber: _logSubscribers) {
      subscriber(level, topic, message);
    }
  }

  #define RHEO_LOGGING_LOG_LEVEL(label, level) void label(std::optional<std::string> topic, std::string message) { log(level, topic, message); }

  RHEO_LOGGING_LOG_LEVEL(error, rheo::logging::LOG_LEVEL_ERROR)
  RHEO_LOGGING_LOG_LEVEL(warn, rheo::logging::LOG_LEVEL_WARN)
  RHEO_LOGGING_LOG_LEVEL(info, rheo::logging::LOG_LEVEL_INFO)
  RHEO_LOGGING_LOG_LEVEL(debug, rheo::logging::LOG_LEVEL_DEBUG)
  RHEO_LOGGING_LOG_LEVEL(trace, rheo::logging::LOG_LEVEL_TRACE)

}