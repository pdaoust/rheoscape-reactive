#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Re-emit a number of values from the source function,
  // then end the source.
  template <typename T>
  RHEO_CALLABLE source_fn<Endable<T>> take(source_fn<T> source, size_t count) {

    struct SourceBinder {
      source_fn<T> source;
      size_t count;

      RHEO_CALLABLE pull_fn operator()(push_fn<Endable<T>> push) const {

        struct PushHandler {
          size_t count;
          push_fn<Endable<T>> push;
          mutable size_t i = 0;

          RHEO_CALLABLE void operator()(T&& value) const {
            if (i < count) {
              push(Endable<T>(std::forward<T>(value), i == count - 1));
              i++;
            } else {
              push(Endable<T>());
            }
          }
        };

        return source(PushHandler{count, push});
      }
    };

    return SourceBinder{source, count};
  }

  namespace detail {
    struct TakePipeFactory {
      size_t count;

      template <typename T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return take(source, count);
      }
    };
  }

  inline auto take(size_t count) {
    return detail::TakePipeFactory{count};
  }

}
