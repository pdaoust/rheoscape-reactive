#pragma once

#include <functional>
#include <utility>
#include <core_types.hpp>

namespace rheo::operators {

  // inspect executes a callback function on each value,
  // then passes the value downstream unchanged.
  // Similar to tee, but takes a simple callback rather than a sink.
  // Useful for logging, debugging, or other side effects.

  // Source function factory - creates a source that executes exec on each value,
  // then passes it downstream.
  // Usage: inspect(source, my_callback)
  template <typename T, typename ExecFn>
    requires concepts::Visitor<ExecFn, T>
  source_fn<T> inspect(source_fn<T> source, ExecFn&& exec) {
    using ExecFnDecayed = std::decay_t<ExecFn>;

    struct SourceBinder {
      source_fn<T> source;
      ExecFnDecayed exec;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push_downstream) const {

        struct PushHandler {
          push_fn<T> push_downstream;
          ExecFnDecayed const& exec;

          RHEO_CALLABLE void operator()(T value) const {
            // Pass as const reference to prevent exec from consuming the value.
            // The value must remain intact for downstream.
            exec(std::as_const(value));
            push_downstream(std::move(value));
          }
        };

        return source(PushHandler{push_downstream, exec});
      }
    };

    return SourceBinder{
      source,
      std::forward<ExecFn>(exec)
    };
  }

  namespace detail {
    template <typename ExecFn>
    struct InspectPipeFactory {
      ExecFn exec;

      template <typename T>
        requires concepts::Visitor<ExecFn, T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
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
