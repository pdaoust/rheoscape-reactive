#pragma once

#include <functional>
#include <fmt/format.h>
#include <core_types.hpp>
#include <Arduino.h>
#ifndef PLATFORMIO
  #include <ArduinoJson.h>
#endif
#if defined(IS_CI)
  #include <IJson.h>
#endif
#include <MQTTRemote.h>
#include <home_assistant/types.hpp>

namespace rheo::home_assistant {

  template <typename T>
  pullable_sink_fn<T> home_assistant_entity(
    source_fn<T> source,
    Entity entity,
    MQTTRemote mqtt
    map
  ) {
    return 
  }
}