#pragma once

#include <types/core_types.hpp>

namespace rheoscape {

  template <typename TVal, typename TTag>
  struct TaggedValue {
    TVal value;
    TTag tag;
  };

}