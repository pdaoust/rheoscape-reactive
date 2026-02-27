#pragma once

#include <type_traits>
#include <types/core_types.hpp>

namespace rheoscape::util {

  // Wraps any callable into a Source without type erasure.
  // Analogous to source_fn<T> but preserves the concrete callable type
  // for inlining and optimization.
  template <typename T, typename Fn>
  struct SourceWrapper {
    using value_type = T;
    Fn fn;

    template <typename PushFn>
      requires concepts::Visitor<PushFn, T>
    RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
      return fn(std::move(push));
    }
  };

  template <typename T, typename Fn>
  auto as_source(Fn fn) {
    return SourceWrapper<T, std::decay_t<Fn>>{std::move(fn)};
  }

}
