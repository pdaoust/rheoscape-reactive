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
    
    void destroy() {
        if (_hasValue) {
            _value.~T();
        } else {
            _error.~TErr();
        }
    }

    public:
      // Constructor for success case.
      Fallible(const T& value) : _hasValue(true), _value(value) {}
      Fallible(T&& value) : _hasValue(true), _value(std::move(value)) {}
    
      // Constructor for error case - explicit constructors to avoid ambiguity
      Fallible(const TErr& error) : _hasValue(false), _error(error) {}
      Fallible(TErr&& error) : _hasValue(false), _error(std::move(error)) {}
    
      // Or alternatively, use tagged constructors for clarity:
      struct ErrorTag {};
      static constexpr ErrorTag error_tag{};
    
      template<typename E>
      Fallible(ErrorTag, E&& error) : _hasValue(false), _error(std::forward<E>(error)) {}

      // Copy constructor
      Fallible(const Fallible& other) : _hasValue(other._hasValue) {
        if (_hasValue) {
          new(&_value) T(other._value);
        } else {
          new(&_error) TErr(other._error);
        }
      }

      // Move constructor
      Fallible(Fallible&& other) noexcept : _hasValue(other._hasValue) {
        if (_hasValue) {
          new(&_value) T(std::move(other._value));
        } else {
          new(&_error) TErr(std::move(other._error));
        }
      }

      // Copy assignment operator
      Fallible& operator=(const Fallible& other) {
        if (this == &other) { return *this; };
        
        // Destroy current object
        destroy();
        
        // Copy construct new object
        _hasValue = other._hasValue;
        if (_hasValue) {
          new(&_value) T(other._value);
        } else {
          new(&_error) TErr(other._error);
        }
        
        return *this;
      }

      // Move assignment operator
      Fallible& operator=(Fallible&& other) noexcept {
        if (this == &other) return *this;
        
        // Destroy current object
        destroy();
        
        // Move construct new object
        _hasValue = other._hasValue;
        if (_hasValue) {
          new(&_value) T(std::move(other._value));
        } else {
          new(&_error) TErr(std::move(other._error));
        }
        
        return *this;
      }

      // Destructor
      ~Fallible() {
        destroy();
      }
    
      bool isOk() {
        return _hasValue;
      }

      bool isError() {
        return !_hasValue;
      }

      T& value() {
        if (!isOk()) {
          throw fallible_bad_get_value_access();
        }
        return _value;
      }

      const T& value() const {
        if (!isOk()) {
          throw fallible_bad_get_value_access();
        }
        return _value;
      }

      TErr& error() {
        if (!isError()) {
          throw fallible_bad_get_error_access();
        }
        return _error;
      }
    
      const TErr& error() const {
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