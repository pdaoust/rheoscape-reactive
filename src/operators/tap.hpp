#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // Tapping a stream is like sharing it,
  // except that there's a 'primary' stream and a 'side' stream,
  // and when you pull on the primary stream,
  // the value that gets pushed down also gets pushed to the side stream.
  // The side stream becomes a push-only stream;
  // for instance, if the sink parameter is a pullable sink function or a pipe function,
  // pulling on it will do nothing.
  // This function is useful for logging.

  template <typename TTapReturn, typename T>
  source_fn<T> tap(source_fn<T> source, sink_fn<TTapReturn, T> sink) {
    return [source, sink](push_fn<T> pushPrimary) {
      push_fn<T> pushToTap;

      sink((source_fn<T>)[&pushToTap](push_fn<T> push) {
        pushToTap = push;
        // The tap isn't allowed to pull.
        return [](){};
      });

      return source([pushPrimary, pushToTap](T value) {
        pushPrimary(value);
        if (pushToTap) {
          pushToTap(value);
        }
      });
    };
  }

  template <typename TTapReturn, typename T>
  pipe_fn<T, T> tap(sink_fn<TTapReturn, T> sink) {
    return [sink](source_fn<T> source) {
      return tap<TTapReturn, T>(source, sink);
    };
  }

}