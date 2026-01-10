#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // Named callable for foreach's push handler
  template<typename T, typename ExecFn>
  struct foreach_push_handler {
    ExecFn exec;

    RHEO_NOINLINE void operator()(T value) const {
      exec(value);
    }
  };

  // Named callable for foreach's source binder
  template<typename T, typename ExecFn>
  struct foreach_source_binder {
    ExecFn exec;

    RHEO_NOINLINE pull_fn operator()(source_fn<T> source) const {
      return source(foreach_push_handler<T, ExecFn>{exec});
    }
  };

  // Sink factory
  template <typename ExecFn>
  auto foreach(ExecFn&& exec)
  -> pullable_sink_fn<arg_of<ExecFn>> {
    using T = arg_of<ExecFn>;
    return foreach_source_binder<T, std::decay_t<ExecFn>>{
      std::forward<ExecFn>(exec)
    };
  }

  template <typename T, typename ExecFn>
    requires concepts::Visitor<ExecFn, T>
  pull_fn foreach(source_fn<T> source, ExecFn&& exec) {
    return foreach(std::forward<ExecFn>(exec))(source);
  }

}