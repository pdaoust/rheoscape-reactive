#pragma once

#include <exception>
#include <map>
#include <vector>
#include <fmt/format.h>
#include <core_types.hpp>
#include <ArduinoJson.h>

namespace rheo::homeAssistant {

  class value_bad_get_type_access : std::exception {
    public:
      const char* what() {
        return "Tried to coerce the value to a wrong type";
      }
  };

  struct Value {
    enum Type { Boolean, String, Integer, Float, List, };
    Type type;

    Value(bool b) : b(b), type(Type::Boolean) { }
    Value(std::string s) : s(s), type(Type::String) { }
    Value(int i) : i(i), type(Type::Integer) { }
    Value(double d) : d(d), type(Type::Float) { }
    Value(std::vector<std::string> l) : l(l), type(Type::List) { }

    bool asBoolean() {
      if (type != Type::Boolean) {
        throw value_bad_get_type_access();
      }
      return b;
    }

    std::string asString() {
      if (type != Type::String) {
        throw value_bad_get_type_access();
      }
      return s;
    }

    int asInteger() {
      if (type != Type::Integer) {
        throw value_bad_get_type_access();
      }
      return i;
    }

    double asFloat() {
      if (type != Type::Float) {
        throw value_bad_get_type_access();
      }
      return d;
    }

    std::vector<std::string> asList() {
      if (type != Type::List) {
        throw value_bad_get_type_access();
      }
      return l;
    }

    private:
      union {
        bool b;
        std::string s;
        int i;
        double d;
        std::vector<std::string> l;
      };
  };

  struct Device {
    std::vector<std::string> identifiers;
    std::string name;
    std::string manufacturer;
    std::string model;
    std::string modelId;
    std::string swVersion;
    std::string hwVersion;
    std::string suggestedArea;
    std::string serialNumber;

    JsonDocument toJson() {
      JsonDocument doc;
      JsonArray identifiers = doc["identifiers"].to<JsonArray>();
      for (auto id: identifiers) {
        identifiers.add(id);
      }
      doc["name"] = name;
      if (!manufacturer.empty()) { doc["manufacturer"] = manufacturer; }
      if (!model.empty()) { doc["model"] = model; }
      if (!modelId.empty()) { doc["model_id"] = modelId; }
      if (!swVersion.empty()) { doc["sw_version"] = swVersion; }
      if (!hwVersion.empty()) { doc["hw_version"] = hwVersion; }
      if (!suggestedArea.empty()) { doc["suggested_area"] = suggestedArea; }
      if (!serialNumber.empty()) { doc["serial_number"] = serialNumber; }
      return doc;
    }
  };

  struct Origin {
    std::string name;
    std::string swVersion;
    std::string supportUrl;

    JsonDocument toJson() {
      JsonDocument doc;
      doc["name"] = name;
      doc["sw_version"] = swVersion;
      doc["support_url"] = supportUrl;   
      return doc;   
    }
  };

  class entity_topic_key_not_found : std::exception {
    private:
      std::string topicType;
      std::string topicKey;

    public:
      entity_topic_key_not_found(std::string topicType, std::string topicKey)
      : topicType(topicType), topicKey(topicKey)
      { }

      const char* what() {
        return fmt::format("Couldn't find {} topic '{}'", topicType.c_str(), topicKey.c_str()).c_str();
      }
  };

  class Entity {
    protected:
      std::string baseTopic() {
        return fmt::format("homeassistant/{}/{}", platform.c_str(), device.identifiers[0].c_str());
      }

    public:
      Device device;
      Origin origin;
      std::string platform;
      // E.g., "temperature" for sensor, "awning" for cover, etc.
      std::string deviceClass;
      std::string id;
      // A map of internal command keys to topic names and keys.
      // Here's the way these should be used.
      // Let's say your entity is a cover and needs to listen to a tilt command.
      // You want to do two things:
      //
      // 1. Assign a map to this field containing `{ { "tilt", { "tilt_command", "tilt_position" } } }`.
      // 2. Add keys and values to your discovery message
      //    to tell HA how to send you command messages (here using ArduinoJSON):
      //
      //    ```c++
      //    doc[entity.commandTopicKey("tilt")] = commandTopic();
      //    doc[entity.commandTemplateKey("tilt")] = commandTemplate("tilt");
      //    ```
      //
      //    This will generate JSON that looks like this:
      //
      //    ```json
      //    {
      //      // ...
      //      // All command types for this entity listen on the same topic
      //      "tilt_command_topic": "/homeassistant/cover/<device.id>/set",
      //      // The internal key of the topic is used in the command payload.
      //      "tilt_command_template": "{\"tilt\":{{ tilt_position }}}",
      //      // ...
      //    }
      // 3. Subscribe to the command topic `commandTopic("tilt")`
      //    and look for JSON messages with the key `"tilt"`:
      //
      //    ```c++
      //    if (doc["tilt"].is<JsonVariant>()) {
      //      // It's a tilt command!
      //    }
      //    ```
      std::map<std::string, std::tuple<std::string, std::string>> commandTopics;

      // A map of internal status keys to topic names.
      // For `{ "tilt", "tilt_status" }` you'd want to:
      //
      // 1. Add the tilt topic and template to your discovery message:
      //
      //    ```c++
      //    doc[entity.stateTopicKey()] = entity.stateTopic();
      //    doc[entity.stateTemplateKey("tilt")] = entity.stateTemplate("tilt");
      //    ```
      // 2. Publish state messages wrapped in a JSON object with the right key:
      //
      //    ```c++
      //    doc["tilt"] = value;
      //    ```
      std::map<std::string, std::string> stateTopics;
      std::map<std::string, Value> platformSpecificOptions;

      std::string formatTopic(std::string topic) {
        return fmt::format("{}/{}", baseTopic().c_str(), topic.c_str());
      }

      std::string discoveryTopic() {
        return formatTopic("config");
      }

      std::string availabilityTopic() {
        return formatTopic("availability");
      }

      std::string commandTopic() {
        return formatTopic("set");
      }

      std::string commandTopicKey(std::string key) {
        if (!commandTopics.count(key)) {
          throw entity_topic_key_not_found("command", key);
        }
        return fmt::format("{}_topic", std::get<0>(commandTopics[key]).c_str());
      }

      std::string commandTemplateKey(std::string key) {
        if (!commandTopics.count(key)) {
          throw entity_topic_key_not_found("command", key);
        }
        return fmt::format("{}_template", std::get<0>(commandTopics[key]).c_str());
      }

      std::string commandTemplate(std::string key) {
        if (!commandTopics.count(key)) {
          throw entity_topic_key_not_found("command", key);
        }
        return fmt::format("\{\"{}\":\{\{{}\}\}\}", key, std::get<1>(commandTopics[key]).c_str());
      }

      std::string stateTopic() {
        return formatTopic("state");
      }

      std::string stateTopicKey(std::string key) {
        if (!stateTopics.count(key)) {
          throw entity_topic_key_not_found("state", key);
        }
        return fmt::format("{}_topic", stateTopics[key].c_str());
      }

      std::string stateTemplateKey(std::string key) {
        if (!stateTopics.count(key)) {
          throw entity_topic_key_not_found("state", key);
        }
        return fmt::format("{}_template", stateTopics[key].c_str());
      }

      std::string stateTemplate(std::string key) {
        if (!stateTopics.count(key)) {
          throw entity_topic_key_not_found("state", key);
        }
        return fmt::format("\{\{value_json.{}\}\}", key);
      }

      JsonDocument toJson() {
        JsonDocument doc;
        doc["device"] = device.toJson();
        doc["origin"] = origin.toJson();
        doc["device_class"] = deviceClass;
        doc["unique_id"] = id;
        for (auto& option : platformSpecificOptions) {
          switch (option.second.type) {
            case Value::Type::Boolean: doc[option.first] = option.second.asBoolean(); break;
            case Value::Type::String:  doc[option.first] = option.second.asString(); break;
            case Value::Type::Integer: doc[option.first] = option.second.asInteger(); break;
            case Value::Type::Float:   doc[option.first] = option.second.asFloat(); break;
            case Value::Type::List:    doc[option.first] = option.second.asList(); break;
          }
        }
        return doc;
      }
  };

}