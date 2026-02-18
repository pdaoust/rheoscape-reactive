#pragma once

namespace rheo {

  // Tag type for the error side of Fallible
  // when raw deserialization (e.g., EEPROM memcpy) produces an invalid value.
  struct deserialization_error {};

}
