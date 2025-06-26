#pragma once

#include <functional>
#include <core_types.hpp>
#include <Fallible.hpp>
#include <operators/filterMap.hpp>
#include <operators/logErrors.hpp>

namespace rheo::operators {
  
  template <typename T, typename TErr>
  source_fn<T> makeInfallible(source_fn<Fallible<T, TErr>> source) {
    return filterMap<T, Fallible<T, TErr>>(source, (map_fn<std::optional<T>, Fallible<T, TErr>>)[](Fallible<T, TErr> value) {
      return value.isOk()
        ? (std::optional<T>)value.value()
        : std::nullopt
      ;
    });
  }

  template <typename T, typename TErr>
  pipe_fn<T, Fallible<T, TErr>> makeInfallible() {
    return [](source_fn<Fallible<T, TErr>> source) {
      return makeInfallible(source);
    };
  }

}