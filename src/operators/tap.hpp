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

  template <typename T, typename SinkFn>
  source_fn<T> tap(source_fn<T> source, SinkFn&& sink) {
    return [source, sink = std::forward<SinkFn>(sink)](push_fn<T> pushPrimary) {
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

  template <typename SinkFn>
  auto tap(SinkFn&& sink)
  -> pipe_fn<
    sink_source_type_t<SinkFn>,
    sink_source_type_t<SinkFn>
  > {
    using T = sink_source_type_t<SinkFn>;
    return [sink = std::forward<SinkFn>(sink)](source_fn<T> source) {
      return tap(source, sink);
    };
  }

}