#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // Sink factory - creates a pullable sink that executes exec on each value.
  // Usage: source | foreach(my_exec)
  //
  // PATTERN NOTE: This is a SINK, not an operator. Sinks consume values without
  // producing a new source. The parameter order for sinks differs from operators:
  // - Operators: operator(source, ...) and operator_with(...) for pipe factory
  // - Sinks: foreach(exec) returns a sink that can be piped

  namespace detail {
    template <typename ExecFn>
    struct ForeachSinkFactory {
      ExecFn exec;

      template <typename T>
        requires concepts::Visitor<ExecFn, T>
      RHEO_CALLABLE pull_fn operator()(source_fn<T> source) const {

        struct PushHandler {
          ExecFn exec;

          RHEO_CALLABLE void operator()(T value) const {
            exec(value);
          }
        };

        return source(PushHandler{exec});
      }
    };
  }

  template <typename ExecFn>
  auto foreach(ExecFn&& exec) {
    return detail::ForeachSinkFactory<std::decay_t<ExecFn>>{
      std::forward<ExecFn>(exec)
    };
  }

  // Convenience overload that immediately binds source to sink.
  // Usage: foreach(source, my_exec) - equivalent to (source | foreach(my_exec))
  template <typename T, typename ExecFn>
    requires concepts::Visitor<ExecFn, T>
  pull_fn foreach(source_fn<T> source, ExecFn&& exec) {
    return foreach(std::forward<ExecFn>(exec))(source);
  }

}
