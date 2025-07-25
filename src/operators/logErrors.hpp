#pragma once

#include <functional>
#include <core_types.hpp>
#include <util.hpp>
#include <Fallible.hpp>
#include <logging.hpp>
#include <operators/tap.hpp>

namespace rheo::operators {

  template <typename T, typename TErr, typename MapFn>
  source_fn<Fallible<T, TErr>> logErrors(
    source_fn<Fallible<T, TErr>> source,
    MapFn&& formatError = [](TErr value) { return string_format("%s", value); },
    const char* topic = NULL
  ) {
    return tap(source, [topic, formatError = std::forward<MapFn>(formatError)](source_fn<Fallible<T, TErr>> source) {
      source([topic, formatError](Fallible<T, TErr> value) {
        if (value.isError()) {
          logging::error(topic, formatError(value.error()));
        }
      });
    });
  }

  template <typename T, typename TErr, typename MapFn>
  pipe_fn<Fallible<T, TErr>, Fallible<T, TErr>> logErrors(
    MapFn&& formatError = [](TErr value) { return string_format("%s", value); },
    const char* topic = NULL
  ) {
    return [formatError, topic](source_fn<Fallible<T, TErr>> source) {
      return logErrors(source, formatError, topic);
    };
  }
  
}