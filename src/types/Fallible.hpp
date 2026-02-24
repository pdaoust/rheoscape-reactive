#pragma once

#include <cassert>
#include <types/core_types.hpp>
#include <operators/filter_map.hpp>

namespace rheoscape {

  template <typename T, typename TErr>
  class Fallible {
    public:
      using ok_type = T;
      using error_type = TErr;

    private:
      bool _has_value;
      union {
        T _value;
        TErr _error;
      };

    void destroy() {
        if (_has_value) {
            _value.~T();
        } else {
            _error.~TErr();
        }
    }

    public:
      // Constructor for success case.
      Fallible(const T& value) : _has_value(true), _value(value) {}
      Fallible(T&& value) : _has_value(true), _value(std::move(value)) {}

      // Constructor for error case - explicit constructors to avoid ambiguity
      Fallible(const TErr& error) : _has_value(false), _error(error) {}
      Fallible(TErr&& error) : _has_value(false), _error(std::move(error)) {}

      // Or alternatively, use tagged constructors for clarity:
      struct ErrorTag {};
      static constexpr ErrorTag error_tag{};

      template<typename E>
      Fallible(ErrorTag, E&& error) : _has_value(false), _error(std::forward<E>(error)) {}

      // Copy constructor
      Fallible(const Fallible& other) : _has_value(other._has_value) {
        if (_has_value) {
          new(&_value) T(other._value);
        } else {
          new(&_error) TErr(other._error);
        }
      }

      // Move constructor
      Fallible(Fallible&& other) noexcept : _has_value(other._has_value) {
        if (_has_value) {
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
        _has_value = other._has_value;
        if (_has_value) {
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
        _has_value = other._has_value;
        if (_has_value) {
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

      bool is_ok() {
        return _has_value;
      }

      bool is_error() {
        return !_has_value;
      }

      T& value() {
        if (!is_ok()) {
          assert("Tried to get a value from a Fallible that contains an error");
        }
        return _value;
      }

      const T& value() const {
        if (!is_ok()) {
          assert("Tried to get a value from a Fallible that contains an error");
        }
        return _value;
      }

      TErr& error() {
        if (!is_error()) {
          assert("Tried to get an error from a Fallible that contains a value");
        }
        return _error;
      }

      const TErr& error() const {
        if (!is_error()) {
          assert("Tried to get an error from a Fallible that contains a value");
        }
        return _error;
      }
  };

  // A filter_map function that extracts values from Fallible, discarding errors.
  template <typename T, typename TErr>
  filter_map_fn<T, Fallible<T, TErr>> filter_map_to_infallible() {
    return [](Fallible<T, TErr> fallible) -> std::optional<T> {
      if (fallible.is_ok()) {
        return std::optional<T>(fallible.value());
      }
      return std::nullopt;
    };
  }

  // Pipe operator that converts a Fallible stream to an infallible stream,
  // filtering out errors and only passing through successful values.
  // Usage: fallible_source | make_infallible<T, TErr>()
  template <typename T, typename TErr>
  pipe_fn<T, Fallible<T, TErr>> make_infallible() {
    return [](source_fn<Fallible<T, TErr>> source) -> source_fn<T> {
      return operators::filter_map(std::move(source), filter_map_to_infallible<T, TErr>());
    };
  }

}