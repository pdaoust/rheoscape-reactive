#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // foreach_passthrough executes an exec function on each value,
  // then passes the value downstream unchanged.
  // Similar to tap, but takes a simple exec function rather than a sink.
  // The exec function receives values by const reference to avoid unnecessary copies.

  // Named callable for foreach_passthrough's push handler
  template<typename T, typename ExecFn>
  struct foreach_passthrough_push_handler {
    push_fn<T> push_downstream;
    ExecFn const& exec;

    RHEO_NOINLINE void operator()(T value) const {
      exec(value);
      push_downstream(std::move(value));
    }
  };

  // Named callable for foreach_passthrough's source binder
  template<typename T, typename ExecFn>
  struct foreach_passthrough_source_binder {
    source_fn<T> source;
    ExecFn exec;

    RHEO_NOINLINE pull_fn operator()(push_fn<T> push_downstream) const {
      return source(foreach_passthrough_push_handler<T, ExecFn>{
        push_downstream,
        exec
      });
    }
  };

  // Source function factory - creates a source that executes exec on each value,
  // then passes it downstream.
  // Usage: foreach_passthrough(source, my_exec)
  template <typename T, typename ExecFn>
    requires concepts::Visitor<ExecFn, T>
  source_fn<T> foreach_passthrough(source_fn<T> source, ExecFn&& exec) {
    return foreach_passthrough_source_binder<T, std::decay_t<ExecFn>>{
      source,
      std::forward<ExecFn>(exec)
    };
  }

  // Pipe function factory - creates a pipe that executes exec on each value,
  // then passes it downstream.
  // Usage: source | foreach_passthrough(my_exec)
  template <typename ExecFn>
  auto foreach_passthrough(ExecFn&& exec)
  -> pipe_fn<arg_of<ExecFn>, arg_of<ExecFn>> {
    using T = arg_of<ExecFn>;
    return [exec = std::forward<ExecFn>(exec)](source_fn<T> source) {
      return foreach_passthrough(source, exec);
    };
  }

}
