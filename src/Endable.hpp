#pragma once

namespace rheo {

  class endable_bad_get_value_access : std::exception {
    public:
      const char* what() {
        return "Tried to get a value from an Endable that's already ended";
      }
  };

  struct Ended {};

  template <typename T>
  class Endable {
    private:
      T _value;
      const bool _isEnded;

    public:
      Endable(const T value)
      : _value(value), _isEnded(false)
      { }

      Endable()
      : _isEnded(true)
      { }

      bool hasValue() const {
        return !_isEnded;
      }

      T value() {
        if (!_isEnded) {
          throw endable_bad_get_value_access();
        }
        return _value;
      }
  };

}