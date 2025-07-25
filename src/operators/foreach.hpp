#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  template <typename ExecFn>
  auto foreach(ExecFn&& exec)
  -> pullable_sink_fn<visitor_1_in_in_type_t<std::decay_t<ExecFn>>> {
    using T = visitor_1_in_in_type_t<std::decay_t<ExecFn>>;
    return [exec = std::forward<ExecFn>(exec)](source_fn<T> source) {
      return source([exec](T value) { exec(value); });
    };
  }

  template <typename T, typename ExecFn>
  pull_fn foreach(source_fn<T> source, ExecFn&& exec) {
    return foreach(std::forward<ExecFn>(exec))(source);
  }

}