#pragma once

#include <functional>
#include <core_types.hpp>
#include <Fallible.hpp>
#include <operators/filterMap.hpp>
#include <operators/logErrors.hpp>

namespace rheo::operators {
  
  template <typename T, typename TErr>
  source_fn<T> makeInfallible(source_fn<Fallible<T, TErr>> source) {
    return filterMap(source, [](Fallible<T, TErr> value) {
      return value.isOk()
        ? (std::optional<T>)value.value()
        : (std::optional<T>)std::nullopt;
    });
  }

  template <typename T, typename TErr>
  pipe_fn<T, Fallible<T, TErr>> makeInfallible() {
    return [](source_fn<Fallible<T, TErr>> source) {
      return makeInfallible(source);
    };
  }

}