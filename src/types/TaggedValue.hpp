#pragma once

#include <core_types.hpp>

namespace rheo {

  template <typename TVal, typename TTag>
  struct TaggedValue {
    const TVal value;
    const TTag tag;

    TaggedValue(TVal value, TTag tag)
    :
      value(value),
      tag(tag)
    { }
  };

}