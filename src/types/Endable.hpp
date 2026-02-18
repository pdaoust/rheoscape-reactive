#pragma once

namespace rheoscape {

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

      Endable(const T value, const bool is_last)
      : _value(value), _status(is_last ? EndableStatus::Last : EndableStatus::NotLast)
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

      bool has_value() const {
        return _status != EndableStatus::Ended;
      }

      T value() {
        if (!has_value()) {
          throw endable_bad_get_value_access();
        }
        return _value;
      }
      
      EndableIsLast is_last() const {
        switch (_status) {
          case EndableStatus::Last: return EndableIsLast::Yes;
          case EndableStatus::NotLast: return EndableIsLast::No;
          case EndableStatus::Indeterminate: return EndableIsLast::Unknowable;
          case EndableStatus::Ended: throw endable_bad_get_value_access();
        }
      }
  };

}