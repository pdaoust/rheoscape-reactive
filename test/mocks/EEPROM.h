#pragma once

// Mock EEPROM for dev_machine tests.
// Provides the subset of the Arduino EEPROM API
// used by EepromState.

#include <array>
#include <cstring>
#include <cstdint>

class MockEEPROMClass {
  static constexpr size_t SIZE = 512;
  std::array<uint8_t, SIZE> _data{};

public:
  MockEEPROMClass() {
    _data.fill(0);
  }

  template <typename T>
  T& get(int address, T& out) {
    std::memcpy(&out, &_data[address], sizeof(T));
    return out;
  }

  template <typename T>
  const T& put(int address, const T& value) {
    std::memcpy(&_data[address], &value, sizeof(T));
    return value;
  }

  uint8_t& operator[](int address) {
    return _data[address];
  }

  // Zero-fill for test reset.
  void clear() {
    _data.fill(0);
  }
};

inline MockEEPROMClass EEPROM;
