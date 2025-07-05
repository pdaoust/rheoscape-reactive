#pragma once

#include <functional>
#include <fmt/format.h>
#include <core_types.hpp>
#include <Arduino.h>
#ifndef PLATFORMIO
  #include <ArduinoJson.h>
#endif
#ifdef IS_CI
  #include <IJson.h>
#endif
#include <MQTTRemote.h>
#include <homeAssistant/types.hpp>

namespace rheo::homeAssistant {

  template <typename T>
  pullable_sink_fn<T> homeAssistantEntity(
    source_fn<T> source,
    Entity entity,
    MQTTRemote mqtt
    map
  ) {
    return 
  }
}