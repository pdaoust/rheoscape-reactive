#pragma once

#include <core_types.hpp>

namespace rheo {

  class fallible_bad_get_value_access : std::exception {
    public:
      const char* what() {
        return "Tried to get a value from a Fallible that contains an error";
      }
  };

  class fallible_bad_get_error_access : std::exception {
    public:
      const char* what() {
        return "Tried to get an error from a Fallible that contains a value";
      }
  };

  template <typename T, typename TErr>
  class Fallible {
    private:
      bool _hasValue;
      union {
        T _value;
        TErr _error;
      };
    
    public:
      Fallible(T value)
      : _hasValue(true), _value(value)
      {}

      Fallible(TErr error)
      : _hasValue(false), _error(error)
      {}

      bool isOk() {
        return _hasValue;
      }

      bool isError() {
        return !_hasValue;
      }

      T value() {
        if (!isOk()) {
          throw fallible_bad_get_value_access();
        }
        return _value;
      }

      TErr error() {
        if (!isError()) {
          throw fallible_bad_get_error_access();
        }
        return _error;
      }
  };

  template <typename T, typename TErr>
  filter_map_fn<T, Fallible<T, TErr>> filterMapToInfallible() {
    return [](Fallible<T, TErr> value) {
      if (value._hasValue) {
        return std::optional<T>(value.getValue());
      }
      return (std::optional<T>)std::nullopt;
    };
  }

}