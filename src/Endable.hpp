#pragma once

namespace rheo {

  class endable_bad_get_value_access : std::exception {
    public:
      const char* what() {
        return "Tried to get a value from an Endable that's already ended";
      }
  };

  struct Ended {};

  enum EndableStatus {
    NotLast,
    Last,
    Ended,
    Indeterminate,
  };

  enum EndableIsLast {
    No = 0,
    Yes = 1,
    Unknowable = -1,
  };

  template <typename T>
  class Endable {
    private:
      T _value;
      EndableStatus _status;

    public:
      Endable(const T value)
      : _value(value), _status(EndableStatus::Indeterminate)
      { }

      Endable(const T value, const bool isLast)
      : _value(value), _status(isLast ? EndableStatus::Last : EndableStatus::NotLast)
      { }

      Endable()
      : _status(EndableStatus::Ended)
      { }

      Endable(const Endable&) = default;
      Endable& operator=(const Endable&) = default;
      Endable(Endable&&) = default;
      Endable& operator=(Endable&&) = default;

      EndableStatus status() const {
        return _status;
      }

      bool hasValue() const {
        return _status != EndableStatus::Ended;
      }

      T value() {
        if (!hasValue()) {
          throw endable_bad_get_value_access();
        }
        return _value;
      }
      
      EndableIsLast isLast() const {
        switch (_status) {
          case EndableStatus::Last: return EndableIsLast::Yes;
          case EndableStatus::NotLast: return EndableIsLast::No;
          case EndableStatus::Indeterminate: return EndableIsLast::Unknowable;
          case EndableStatus::Ended: throw endable_bad_get_value_access();
        }
      }
  };

}