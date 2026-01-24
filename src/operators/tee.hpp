#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/Wrapper.hpp>

namespace rheo::operators {

  // Tee-ing a stream is like sharing it,
  // except that there's a 'primary' stream and a 'side' stream,
  // and when you pull on the primary stream,
  // the value that gets pushed down also gets pushed to the side stream.
  // The side stream becomes a push-only stream;
  // for instance, if the sink parameter is a pullable sink function or a pipe function,
  // pulling on it will do nothing.
  // This function is useful for logging or forking data to another pipeline.

  // Named callable for tee's secondary source (push-only, no pull)
  template<typename T>
  struct tee_secondary_source {
    std::shared_ptr<Wrapper<push_fn<T>>> push_secondary;

    RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
      (*push_secondary).value = std::forward<push_fn<T>>(push);
      // The tee isn't allowed to pull
      return [](){};
    }
  };

  // Named callable for tee's primary push handler
  template<typename T>
  struct tee_push_handler {
    push_fn<T> push_primary;
    std::shared_ptr<Wrapper<push_fn<T>>> push_secondary;

    RHEO_CALLABLE void operator()(T value) const {
      push_primary(value);
      (*push_secondary).value(value);
    }
  };

  // Named callable for tee's source binder
  template<typename T, typename SinkFn>
  struct tee_source_binder {
    source_fn<T> source;
    SinkFn sink;

    RHEO_CALLABLE pull_fn operator()(push_fn<T> push_primary) const {
      auto push_secondary = make_wrapper_shared<push_fn<T>>();
      sink(tee_secondary_source<T>{push_secondary});

      return source(tee_push_handler<T>{push_primary, push_secondary});
    }
  };

  template <typename T, typename SinkFn>
  RHEO_CALLABLE source_fn<T> tee(source_fn<T> source, SinkFn&& sink) {
    return tee_source_binder<T, std::decay_t<SinkFn>>{
      source,
      std::forward<SinkFn>(sink)
    };
  }

  template <typename SinkFn>
  auto tee(SinkFn&& sink)
  -> pipe_fn<
    sink_source_type_t<SinkFn>,
    sink_source_type_t<SinkFn>
  > {
    using T = sink_source_type_t<SinkFn>;
    return [sink = std::forward<SinkFn>(sink)](source_fn<T> source) {
      return tee(source, sink);
    };
  }

}
