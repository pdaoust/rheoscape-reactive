#pragma once

#include <core_types.hpp>

namespace rheo {

  template <typename TVal, typename TTag>
  struct TaggedValue {
    TVal value;
    TTag tag;
  };

}