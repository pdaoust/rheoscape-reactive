#pragma once

#include <functional>
#include <utility>
#include <types/core_types.hpp>

namespace rheoscape::operators {

  // inspect executes a callback function on each value,
  // then passes the value downstream unchanged.
  // Similar to tee, but takes a simple callback rather than a sink.
  // Useful for logging, debugging, or other side effects.

  namespace detail {
    template <typename SourceT, typename ExecFnT>
    struct InspectSourceBinder {
      using value_type = source_value_t<SourceT>;

      SourceT source;
      ExecFnT exec;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push_downstream) const {
        using T = value_type;

        struct PushHandler {
          PushFn push_downstream;
          ExecFnT exec;

          RHEOSCAPE_CALLABLE void operator()(T value) const {
            // Pass as const reference to prevent exec from consuming the value.
            // The value must remain intact for downstream.
            invoke_maybe_apply(exec, std::as_const(value));
            push_downstream(std::move(value));
          }
        };

        return source(PushHandler{std::move(push_downstream), exec});
      }
    };
  }

  // Source function factory - creates a source that executes exec on each value,
  // then passes it downstream.
  // Usage: inspect(source, my_callback)
  template <typename SourceT, typename ExecFn>
    requires concepts::Source<SourceT> && concepts::Visitor<ExecFn, source_value_t<SourceT>>
  RHEOSCAPE_CALLABLE auto inspect(SourceT source, ExecFn&& exec) {
    return detail::InspectSourceBinder<SourceT, std::decay_t<ExecFn>>{
      std::move(source),
      std::forward<ExecFn>(exec)
    };
  }

  namespace detail {
    template <typename ExecFn>
    struct InspectPipeFactory {
      ExecFn exec;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::Visitor<ExecFn, source_value_t<SourceT>>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return inspect(std::move(source), ExecFn(exec));
      }
    };
  }

  template <typename ExecFn>
  auto inspect(ExecFn&& exec) {
    return detail::InspectPipeFactory<std::decay_t<ExecFn>>{
      std::forward<ExecFn>(exec)
    };
  }

}
