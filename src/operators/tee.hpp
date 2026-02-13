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

  template <typename T, typename SinkFn>
  RHEO_CALLABLE source_fn<T> tee(source_fn<T> source, SinkFn&& sink) {
    using SinkFnDecayed = std::decay_t<SinkFn>;

    struct SourceBinder {
      source_fn<T> source;
      SinkFnDecayed sink;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push_primary) const {
        auto push_secondary = make_wrapper_shared<push_fn<T>>();

        struct SecondarySource {
          std::shared_ptr<Wrapper<push_fn<T>>> push_secondary;

          RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
            (*push_secondary).value = std::forward<push_fn<T>>(push);
            // The tee isn't allowed to pull
            return [](){};
          }
        };

        struct PushHandler {
          push_fn<T> push_primary;
          std::shared_ptr<Wrapper<push_fn<T>>> push_secondary;

          RHEO_CALLABLE void operator()(T value) const {
            push_primary(value);
            (*push_secondary).value(value);
          }
        };

        sink(SecondarySource{push_secondary});

        return source(PushHandler{push_primary, push_secondary});
      }
    };

    return SourceBinder{
      source,
      std::forward<SinkFn>(sink)
    };
  }

  namespace detail {
    template <typename SinkFn>
    struct TeePipeFactory {
      SinkFn sink;

      template <typename T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return tee(std::move(source), SinkFn(sink));
      }
    };
  }

  template <typename SinkFn>
  auto tee(SinkFn&& sink) {
    return detail::TeePipeFactory<std::decay_t<SinkFn>>{
      std::forward<SinkFn>(sink)
    };
  }

}
