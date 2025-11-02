#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Wrapper.hpp>

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
      auto pushSecondary = make_wrapper_shared<push_fn<T>>();
      sink((source_fn<T>)[pushSecondary](push_fn<T> push) {
        (*pushSecondary).value = std::forward<push_fn<T>>(push);
        // The tap isn't allowed to pull.
        return [](){};
      });

      return source([pushPrimary, pushSecondary](T value) {
        pushPrimary(value);
        (*pushSecondary).value(value);
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