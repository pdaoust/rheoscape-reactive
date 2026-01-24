#pragma once

#include <functional>
#include <utility>
#include <core_types.hpp>

namespace rheo::operators {

  // inspect executes a callback function on each value,
  // then passes the value downstream unchanged.
  // Similar to tee, but takes a simple callback rather than a sink.
  // Useful for logging, debugging, or other side effects.

  // Named callable for inspect's push handler
  template<typename T, typename ExecFn>
  struct inspect_push_handler {
    push_fn<T> push_downstream;
    ExecFn const& exec;

    RHEO_NOINLINE void operator()(T value) const {
      // Pass as const reference to prevent exec from consuming the value.
      // The value must remain intact for downstream.
      exec(std::as_const(value));
      push_downstream(std::move(value));
    }
  };

  // Named callable for inspect's source binder
  template<typename T, typename ExecFn>
  struct inspect_source_binder {
    source_fn<T> source;
    ExecFn exec;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push_downstream) const {
      return source(inspect_push_handler<T, ExecFn>{
        push_downstream,
        exec
      });
    }
  };

  // Source function factory - creates a source that executes exec on each value,
  // then passes it downstream.
  // Usage: inspect(source, my_callback)
  template <typename T, typename ExecFn>
    requires concepts::Visitor<ExecFn, T>
  source_fn<T> inspect(source_fn<T> source, ExecFn&& exec) {
    return inspect_source_binder<T, std::decay_t<ExecFn>>{
      source,
      std::forward<ExecFn>(exec)
    };
  }

  // Pipe function factory - creates a pipe that executes exec on each value,
  // then passes it downstream.
  // Usage: source | inspect(my_callback)
  template <typename ExecFn>
  auto inspect(ExecFn&& exec)
  -> pipe_fn<arg_of<ExecFn>, arg_of<ExecFn>> {
    using T = arg_of<ExecFn>;
    return [exec = std::forward<ExecFn>(exec)](source_fn<T> source) {
      return inspect(source, exec);
    };
  }

}
