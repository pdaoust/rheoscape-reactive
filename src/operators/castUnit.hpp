#ifndef RHEOSCAPE_CAST_UNIT
#define RHEOSCAPE_CAST_UNIT

#include <functional>
#include <types/au_noio.hpp>
#include <core_types.hpp>
#include <operators/map.hpp>

template <typename TUnit, typename TIn, typename TOut>
map_fn<au::Quantity<TUnit, TIn>, au::Quantity<TUnit, TOut>> castUnit() {
  return au::rep_cast<TOut, TUnit, TIn>;
}

#endif