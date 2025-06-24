#pragma once

#include <functional>
#include <core_types.hpp>
#include <util.hpp>
#include <Fallible.hpp>
#include <logging.hpp>
#include <operators/tap.hpp>

namespace rheo::operators {

  template <typename T, typename TErr>
  source_fn<Fallible<T, TErr>> logErrors(
    source_fn<Fallible<T, TErr>> source,
    map_fn<const char*, TErr> formatError = [](TErr value) { return string_format("%s", value); },
    const char* topic = NULL
  ) {
    return tap<Fallible<T, TErr>, void>(source, [topic, formatError](source_fn<Fallible<T, TErr>> source) {
      source([topic, formatError](Fallible<T, TErr> value) {
        if (value.isError()) {
          logging::error(topic, formatError(value.error()));
        }
      });
    });
  }

  template <typename T, typename TErr>
  pipe_fn<Fallible<T, TErr>, Fallible<T, TErr>> logErrors(
    map_fn<const char*, TErr> formatError = [](TErr value) { return string_format("%s", value); },
    const char* topic = NULL
  ) {
    return [formatError, topic](source_fn<Fallible<T, TErr>> source) {
      return logErrors(source, formatError, topic);
    };
  }
  
}