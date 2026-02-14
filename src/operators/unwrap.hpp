#pragma once

#include <functional>
#include <core_types.hpp>
#include <operators/filter_map.hpp>
#include <Fallible.hpp>
#include <Endable.hpp>

namespace rheo::operators {

  // Type traits for detecting wrapper types
  template <typename T> struct is_endable : std::false_type {};
  template <typename T> struct is_endable<Endable<T>> : std::true_type {};
  template <typename T> inline constexpr bool is_endable_v = is_endable<T>::value;

  template <typename T> struct endable_inner {};
  template <typename T> struct endable_inner<Endable<T>> { using type = T; };
  template <typename T> using endable_inner_t = typename endable_inner<T>::type;

  template <typename T> struct is_fallible : std::false_type {};
  template <typename T, typename TErr> struct is_fallible<Fallible<T, TErr>> : std::true_type {};
  template <typename T> inline constexpr bool is_fallible_v = is_fallible<T>::value;

  template <typename T> struct fallible_inner {};
  template <typename T, typename TErr> struct fallible_inner<Fallible<T, TErr>> {
    using value_type = T;
    using error_type = TErr;
  };

  // ---- unwrap_optional ----

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

  // Generic overload: accepts any Source whose value_type is std::optional<T>
  template <typename SourceT>
    requires (concepts::Source<SourceT> && is_optional_v<source_value_t<SourceT>>
              && !std::is_same_v<std::decay_t<SourceT>, source_fn<source_value_t<SourceT>>>)
  RHEO_CALLABLE auto unwrap_optional(SourceT&& source) {
    using ValT = source_value_t<SourceT>;
    return unwrap_optional(source_fn<ValT>(std::forward<SourceT>(source)));
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

  // ---- unwrap_fallible ----

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

  // Generic overload: accepts any Source whose value_type is Fallible<T, TErr>
  template <typename SourceT>
    requires (concepts::Source<SourceT> && is_fallible_v<source_value_t<SourceT>>
              && !std::is_same_v<std::decay_t<SourceT>, source_fn<source_value_t<SourceT>>>)
  RHEO_CALLABLE auto unwrap_fallible(SourceT&& source) {
    using ValT = source_value_t<SourceT>;
    return unwrap_fallible(source_fn<ValT>(std::forward<SourceT>(source)));
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

  // ---- unwrap_endable ----

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

  // Generic overload: accepts any Source whose value_type is Endable<T>
  template <typename SourceT>
    requires (concepts::Source<SourceT> && is_endable_v<source_value_t<SourceT>>
              && !std::is_same_v<std::decay_t<SourceT>, source_fn<source_value_t<SourceT>>>)
  RHEO_CALLABLE auto unwrap_endable(SourceT&& source) {
    using ValT = source_value_t<SourceT>;
    return unwrap_endable(source_fn<ValT>(std::forward<SourceT>(source)));
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
