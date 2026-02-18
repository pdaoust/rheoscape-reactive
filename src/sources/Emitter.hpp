#pragma once

#include <functional>
#include <types/core_types.hpp>

namespace rheoscape::sources {

  // Forward declaration
  template <typename T>
  class Emitter;

  // Named callable for Emitter's noop pull handler
  struct emitter_noop_pull_handler {
    RHEOSCAPE_CALLABLE void operator()() const {}
  };

  // Named callable for Emitter's source binder
  template <typename T>
  struct emitter_source_binder {
    using value_type = T;
    Emitter<T>* emitter;

    template <typename PushFn>
      requires concepts::Visitor<PushFn, T>
    RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
      return emitter->add_sink(std::move(push));
    }
  };

  template <typename T>
  class Emitter {
    private:
      std::vector<push_fn<T>> _sinks;

    public:
      Emitter() {}

      void push(T value) {
        for (auto& sink : _sinks) {
          sink(value);
        }
      }

      // This can be used as-is as a source function.
      // The PushFn is type-erased into push_fn<T> for storage
      // in the internal sinks vector.
      template <typename PushFn>
        requires concepts::Visitor<PushFn, T>
      auto add_sink(PushFn push) {
        _sinks.push_back(push_fn<T>(push));
        return emitter_noop_pull_handler{};
      }

      auto get_source_fn() {
        return emitter_source_binder<T>{this};
      }
  };

}
