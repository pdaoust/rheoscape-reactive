#pragma once

#include <exception>
#include <map>
#include <vector>
#include <fmt/format.h>
#include <types/core_types.hpp>
#include <ArduinoJson.h>

namespace rheoscape::home_assistant {

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

    bool as_boolean() {
      if (type != Type::Boolean) {
        throw value_bad_get_type_access();
      }
      return b;
    }

    std::string as_string() {
      if (type != Type::String) {
        throw value_bad_get_type_access();
      }
      return s;
    }

    int as_integer() {
      if (type != Type::Integer) {
        throw value_bad_get_type_access();
      }
      return i;
    }

    double as_float() {
      if (type != Type::Float) {
        throw value_bad_get_type_access();
      }
      return d;
    }

    std::vector<std::string> as_list() {
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
    std::string model_id;
    std::string sw_version;
    std::string hw_version;
    std::string suggested_area;
    std::string serial_number;

    JsonDocument to_json() {
      JsonDocument doc;
      JsonArray identifiers = doc["identifiers"].to<JsonArray>();
      for (auto id: identifiers) {
        identifiers.add(id);
      }
      doc["name"] = name;
      if (!manufacturer.empty()) { doc["manufacturer"] = manufacturer; }
      if (!model.empty()) { doc["model"] = model; }
      if (!model_id.empty()) { doc["model_id"] = model_id; }
      if (!sw_version.empty()) { doc["sw_version"] = sw_version; }
      if (!hw_version.empty()) { doc["hw_version"] = hw_version; }
      if (!suggested_area.empty()) { doc["suggested_area"] = suggested_area; }
      if (!serial_number.empty()) { doc["serial_number"] = serial_number; }
      return doc;
    }
  };

  struct Origin {
    std::string name;
    std::string sw_version;
    std::string support_url;

    JsonDocument to_json() {
      JsonDocument doc;
      doc["name"] = name;
      doc["sw_version"] = sw_version;
      doc["support_url"] = support_url;   
      return doc;   
    }
  };

  class entity_topic_key_not_found : std::exception {
    private:
      std::string topic_type;
      std::string topic_key;

    public:
      entity_topic_key_not_found(std::string topic_type, std::string topic_key)
      : topic_type(topic_type), topic_key(topic_key)
      { }

      const char* what() {
        return fmt::format("Couldn't find {} topic '{}'", topic_type.c_str(), topic_key.c_str()).c_str();
      }
  };

  class Entity {
    protected:
      std::string base_topic() {
        return fmt::format("homeassistant/{}/{}", platform.c_str(), device.identifiers[0].c_str());
      }

    public:
      Device device;
      Origin origin;
      std::string platform;
      // E.g., "temperature" for sensor, "awning" for cover, etc.
      std::string device_class;
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
      //    doc[entity.command_topic_key("tilt")] = command_topic();
      //    doc[entity.command_template_key("tilt")] = command_template("tilt");
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
      // 3. Subscribe to the command topic `command_topic("tilt")`
      //    and look for JSON messages with the key `"tilt"`:
      //
      //    ```c++
      //    if (doc["tilt"].is<JsonVariant>()) {
      //      // It's a tilt command!
      //    }
      //    ```
      std::map<std::string, std::tuple<std::string, std::string>> command_topics;

      // A map of internal status keys to topic names.
      // For `{ "tilt", "tilt_status" }` you'd want to:
      //
      // 1. Add the tilt topic and template to your discovery message:
      //
      //    ```c++
      //    doc[entity.state_topic_key()] = entity.state_topic();
      //    doc[entity.state_template_key("tilt")] = entity.state_template("tilt");
      //    ```
      // 2. Publish state messages wrapped in a JSON object with the right key:
      //
      //    ```c++
      //    doc["tilt"] = value;
      //    ```
      std::map<std::string, std::string> state_topics;
      std::map<std::string, Value> platform_specific_options;

      std::string format_topic(std::string topic) {
        return fmt::format("{}/{}", base_topic().c_str(), topic.c_str());
      }

      std::string discovery_topic() {
        return format_topic("config");
      }

      std::string availability_topic() {
        return format_topic("availability");
      }

      std::string command_topic() {
        return format_topic("set");
      }

      std::string command_topic_key(std::string key) {
        if (!command_topics.count(key)) {
          throw entity_topic_key_not_found("command", key);
        }
        return fmt::format("{}_topic", std::get<0>(command_topics[key]).c_str());
      }

      std::string command_template_key(std::string key) {
        if (!command_topics.count(key)) {
          throw entity_topic_key_not_found("command", key);
        }
        return fmt::format("{}_template", std::get<0>(command_topics[key]).c_str());
      }

      std::string command_template(std::string key) {
        if (!command_topics.count(key)) {
          throw entity_topic_key_not_found("command", key);
        }
        return fmt::format("\{\"{}\":\{\{{}\}\}\}", key, std::get<1>(command_topics[key]).c_str());
      }

      std::string state_topic() {
        return format_topic("state");
      }

      std::string state_topic_key(std::string key) {
        if (!state_topics.count(key)) {
          throw entity_topic_key_not_found("state", key);
        }
        return fmt::format("{}_topic", state_topics[key].c_str());
      }

      std::string state_template_key(std::string key) {
        if (!state_topics.count(key)) {
          throw entity_topic_key_not_found("state", key);
        }
        return fmt::format("{}_template", state_topics[key].c_str());
      }

      std::string state_template(std::string key) {
        if (!state_topics.count(key)) {
          throw entity_topic_key_not_found("state", key);
        }
        return fmt::format("\{\{value_json.{}\}\}", key);
      }

      JsonDocument to_json() {
        JsonDocument doc;
        doc["device"] = device.to_json();
        doc["origin"] = origin.to_json();
        doc["device_class"] = device_class;
        doc["unique_id"] = id;
        for (auto& option : platform_specific_options) {
          switch (option.second.type) {
            case Value::Type::Boolean: doc[option.first] = option.second.as_boolean(); break;
            case Value::Type::String:  doc[option.first] = option.second.as_string(); break;
            case Value::Type::Integer: doc[option.first] = option.second.as_integer(); break;
            case Value::Type::Float:   doc[option.first] = option.second.as_float(); break;
            case Value::Type::List:    doc[option.first] = option.second.as_list(); break;
          }
        }
        return doc;
      }
  };

}