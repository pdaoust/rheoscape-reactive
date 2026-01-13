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

  // Named callable for tap's secondary source (push-only, no pull)
  template<typename T>
  struct tap_secondary_source {
    std::shared_ptr<Wrapper<push_fn<T>>> push_secondary;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push) const {
      (*push_secondary).value = std::forward<push_fn<T>>(push);
      // The tap isn't allowed to pull
      return [](){};
    }
  };

  // Named callable for tap's primary push handler
  template<typename T>
  struct tap_push_handler {
    push_fn<T> push_primary;
    std::shared_ptr<Wrapper<push_fn<T>>> push_secondary;

    RHEO_NOINLINE void operator()(T value) const {
      push_primary(value);
      (*push_secondary).value(value);
    }
  };

  // Named callable for tap's source binder
  template<typename T, typename SinkFn>
  struct tap_source_binder {
    source_fn<T> source;
    SinkFn sink;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push_primary) const {
      auto push_secondary = make_wrapper_shared<push_fn<T>>();
      sink(tap_secondary_source<T>{push_secondary});

      return source(tap_push_handler<T>{push_primary, push_secondary});
    }
  };

  template <typename T, typename SinkFn>
  RHEO_INLINE source_fn<T> tap(source_fn<T> source, SinkFn&& sink) {
    return tap_source_binder<T, std::decay_t<SinkFn>>{
      source,
      std::forward<SinkFn>(sink)
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