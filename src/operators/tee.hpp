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

  namespace detail {
    template <typename SourceT, typename SinkFnT>
    struct TeeSourceBinder {
      using value_type = source_value_t<SourceT>;

      SourceT source;
      SinkFnT sink;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push_primary) const {
        using T = value_type;
        auto push_secondary = make_wrapper_shared<push_fn<T>>();

        // SecondarySource is a mini-source that the sink binds to.
        // It stores the secondary push in a shared_ptr so PushHandler can use it.
        // Uses push_fn<T> (type-erased) because the sink may provide any push type.
        struct SecondarySource {
          using value_type = T;
          std::shared_ptr<Wrapper<push_fn<T>>> push_secondary;

          RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
            (*push_secondary).value = std::move(push);
            // The tee isn't allowed to pull
            return [](){};
          }
        };

        struct PushHandler {
          PushFn push_primary;
          std::shared_ptr<Wrapper<push_fn<T>>> push_secondary;

          RHEO_CALLABLE void operator()(T value) const {
            push_primary(value);
            (*push_secondary).value(value);
          }
        };

        sink(SecondarySource{push_secondary});

        return source(PushHandler{std::move(push_primary), push_secondary});
      }
    };
  }

  template <typename SourceT, typename SinkFn>
    requires concepts::Source<SourceT>
  RHEO_CALLABLE auto tee(SourceT source, SinkFn&& sink) {
    return detail::TeeSourceBinder<SourceT, std::decay_t<SinkFn>>{
      std::move(source),
      std::forward<SinkFn>(sink)
    };
  }

  namespace detail {
    template <typename SinkFn>
    struct TeePipeFactory {
      SinkFn sink;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEO_CALLABLE auto operator()(SourceT source) const {
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
