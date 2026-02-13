#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/filter_map.hpp>
#include <Fallible.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  template <typename T>
  RHEO_CALLABLE source_fn<T> unwrap_optional(source_fn<std::optional<T>> source) {

    struct SourceBinder {
      source_fn<std::optional<T>> source;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PushHandler {
          push_fn<T> push;

          RHEO_CALLABLE void operator()(std::optional<T> value) const {
            if (value.has_value()) {
              push(value.value());
            }
          }
        };

        return source(PushHandler{push});
      }
    };

    return SourceBinder{source};
  }

  namespace detail {
    struct UnwrapOptionalPipeFactory {
      template <typename TIn>
        requires is_optional_v<TIn>
      RHEO_CALLABLE auto operator()(source_fn<TIn> source) const {
        return unwrap_optional(std::move(source));
      }
    };
  }

  inline auto unwrap_optional() {
    return detail::UnwrapOptionalPipeFactory{};
  }

  template <typename T, typename TErr>
  RHEO_CALLABLE source_fn<T> unwrap_fallible(source_fn<Fallible<T, TErr>> source) {

    struct SourceBinder {
      source_fn<Fallible<T, TErr>> source;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PushHandler {
          push_fn<T> push;

          RHEO_CALLABLE void operator()(Fallible<T, TErr> value) const {
            if (value.is_ok()) {
              push(value);
            }
          }
        };

        return source(PushHandler{push});
      }
    };

    return SourceBinder{source};
  }

  // unwrap_fallible pipe factory still needs explicit template args
  // because we can't deduce whether TIn is Fallible<T, TErr> generically.
  template <typename T, typename TErr>
  pipe_fn<T, Fallible<T, TErr>> unwrap_fallible() {

    struct PipeFactory {
      RHEO_CALLABLE source_fn<T> operator()(source_fn<Fallible<T, TErr>> source) const {
        return unwrap_fallible(source);
      }
    };

    return PipeFactory{};
  }

  template <typename T>
  RHEO_CALLABLE source_fn<T> unwrap_endable(source_fn<Endable<T>> source) {

    struct SourceBinder {
      source_fn<Endable<T>> source;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PushHandler {
          push_fn<T> push;

          RHEO_CALLABLE void operator()(Endable<T> value) const {
            if (value.has_value()) {
              push(value.value());
            }
          }
        };

        return source(PushHandler{push});
      }
    };

    return SourceBinder{source};
  }

  // unwrap_endable pipe factory still needs explicit template args
  // because we can't deduce whether TIn is Endable<T> generically.
  template <typename T>
  pipe_fn<T, Endable<T>> unwrap_endable() {

    struct PipeFactory {
      RHEO_CALLABLE source_fn<T> operator()(source_fn<Endable<T>> source) const {
        return unwrap_endable(source);
      }
    };

    return PipeFactory{};
  }

}
