#pragma once

#include <functional>
#include <cstdarg>
#include <util.hpp>
#include <Arduino.h>

namespace rheo::logging {

  const uint8_t LOG_LEVEL_SILENT = 0;
  const uint8_t LOG_LEVEL_ERROR = 1;
  const uint8_t LOG_LEVEL_WARN = 2;
  const uint8_t LOG_LEVEL_INFO = 3;
  const uint8_t LOG_LEVEL_DEBUG = 4;
  const uint8_t LOG_LEVEL_TRACE = 5;
  #define LOG_LEVEL_LABEL(l) (l == rheo::logging::LOG_LEVEL_ERROR ? "ERROR" : l == rheo::logging::LOG_LEVEL_WARN ? "WARN" : l == rheo::logging::LOG_LEVEL_INFO ? "INFO" : l == rheo::logging::LOG_LEVEL_DEBUG ? "DEBUG" : l == rheo::logging::LOG_LEVEL_TRACE ? "TRACE" : "")

  namespace {
    static std::vector<std::function<void(uint8_t, const char* const, const char* const)>> _logSubscribers;
  }

  void registerSubscriber(std::function<void(uint8_t, const char* const, const char* const)> subscriber) {
    _logSubscribers.push_back(subscriber);
  }

  void log(uint8_t level, const char* const topic, const char* const message, ...) {
    va_list vaArgs;
    va_start(vaArgs, message);
    va_end(vaArgs);
    va_list vaArgsCopy;
    va_copy(vaArgsCopy, vaArgs);
    const int iLen = std::vsnprintf(NULL, 0, message, vaArgsCopy);
    va_end(vaArgsCopy);
    std::vector<char> zc(iLen + 1);
    std::vsnprintf(zc.data(), zc.size(), message, vaArgs);
    va_end(vaArgs);
    for (auto subscriber: _logSubscribers) {
      subscriber(level, topic, zc.data());
    }
  }

  #define RHEO_LOGGING_LOG_LEVEL(label, level) void label(const char* const topic, const char* const message, ...) { va_list vaArgs; va_start(vaArgs, message); log(level, topic, message, vaArgs); va_end(vaArgs); }

  RHEO_LOGGING_LOG_LEVEL(error, rheo::logging::LOG_LEVEL_ERROR)
  RHEO_LOGGING_LOG_LEVEL(warn, rheo::logging::LOG_LEVEL_WARN)
  RHEO_LOGGING_LOG_LEVEL(info, rheo::logging::LOG_LEVEL_INFO)
  RHEO_LOGGING_LOG_LEVEL(debug, rheo::logging::LOG_LEVEL_DEBUG)
  RHEO_LOGGING_LOG_LEVEL(trace, rheo::logging::LOG_LEVEL_TRACE)

  // void error(const char* const topic, const char* const message, ...) {
  //   va_list vaArgs;
  //   // Serial.println("Got this far 0000");
  //   va_start(vaArgs, message);
  //   log(LOG_LEVEL_ERROR, topic, message);
  // }

  // void warn(const char* const topic, const char* const message, ...) {
  //   log(LOG_LEVEL_WARN, topic, message);
  // }

  // void info(const char* const topic, const char* const message, ...) {
  //   log(LOG_LEVEL_INFO, topic, message);
  // }

  // void debug(const char* const topic, const char* const message, ...) {
  //   log(LOG_LEVEL_DEBUG, topic, message);
  // }

  // void trace(const char* const topic, const char* const message, ...) {
  //   log(LOG_LEVEL_TRACE, topic, message);
  // }
}