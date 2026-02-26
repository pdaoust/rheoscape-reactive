#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <types/Endable.hpp>

namespace rheoscape::operators {

  namespace detail {
    template <typename SourceT>
    struct TakeSourceBinder {
      using T = source_value_t<SourceT>;
      using value_type = Endable<T>;

      SourceT source;
      size_t count;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          size_t count;
          PushFn push;
          mutable size_t i = 0;

          RHEOSCAPE_CALLABLE void operator()(T value) const {
            if (i < count) {
              push(Endable<T>(std::move(value), i == count - 1));
              i++;
            } else {
              push(Endable<T>());
            }
          }
        };

        return source(PushHandler{count, std::move(push)});
      }
    };
  }

  // Re-emit a number of values from the source function,
  // then end the source.
  template <typename SourceT>
    requires concepts::Source<SourceT>
  RHEOSCAPE_CALLABLE auto take(SourceT source, size_t count) {
    return detail::TakeSourceBinder<SourceT>{std::move(source), count};
  }

  namespace detail {
    struct TakePipeFactory {
      using is_pipe_factory = void;
      size_t count;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return take(std::move(source), count);
      }
    };
  }

  inline auto take(size_t count) {
    return detail::TakePipeFactory{count};
  }

}
