#pragma once

#include <functional>
#include <types/core_types.hpp>
#include <operators/filter_map.hpp>
#include <types/Fallible.hpp>
#include <types/Endable.hpp>

namespace rheoscape::operators {

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

  namespace detail {
    template <typename SourceT>
    struct UnwrapOptionalSourceBinder {
      using OptT = source_value_t<SourceT>;
      using value_type = typename OptT::value_type;

      SourceT source;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        using T = value_type;

        struct PushHandler {
          PushFn push;

          RHEOSCAPE_CALLABLE void operator()(OptT value) const {
            if (value.has_value()) {
              push(value.value());
            }
          }
        };

        return source(PushHandler{std::move(push)});
      }
    };
  }

  template <typename SourceT>
    requires concepts::Source<SourceT> && is_optional_v<source_value_t<SourceT>>
  RHEOSCAPE_CALLABLE auto unwrap_optional(SourceT source) {
    return detail::UnwrapOptionalSourceBinder<SourceT>{std::move(source)};
  }

  namespace detail {
    struct UnwrapOptionalPipeFactory {
      using is_pipe_factory = void;
      template <typename SourceT>
        requires concepts::Source<SourceT> && is_optional_v<source_value_t<SourceT>>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return unwrap_optional(std::move(source));
      }
    };
  }

  inline auto unwrap_optional() {
    return detail::UnwrapOptionalPipeFactory{};
  }

  // ---- unwrap_fallible ----

  namespace detail {
    template <typename SourceT>
    struct UnwrapFallibleSourceBinder {
      using FallibleT = source_value_t<SourceT>;
      using value_type = typename fallible_inner<FallibleT>::value_type;

      SourceT source;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        using T = value_type;

        struct PushHandler {
          PushFn push;

          RHEOSCAPE_CALLABLE void operator()(FallibleT value) const {
            if (value.is_ok()) {
              push(value);
            }
          }
        };

        return source(PushHandler{std::move(push)});
      }
    };
  }

  template <typename SourceT>
    requires concepts::Source<SourceT> && is_fallible_v<source_value_t<SourceT>>
  RHEOSCAPE_CALLABLE auto unwrap_fallible(SourceT source) {
    return detail::UnwrapFallibleSourceBinder<SourceT>{std::move(source)};
  }

  namespace detail {
    struct UnwrapFalliblePipeFactory {
      using is_pipe_factory = void;
      template <typename SourceT>
        requires concepts::Source<SourceT> && is_fallible_v<source_value_t<SourceT>>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return unwrap_fallible(std::move(source));
      }
    };
  }

  inline auto unwrap_fallible() {
    return detail::UnwrapFalliblePipeFactory{};
  }

  // ---- unwrap_endable ----

  namespace detail {
    template <typename SourceT>
    struct UnwrapEndableSourceBinder {
      using EndableT = source_value_t<SourceT>;
      using value_type = endable_inner_t<EndableT>;

      SourceT source;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        using T = value_type;

        struct PushHandler {
          PushFn push;

          RHEOSCAPE_CALLABLE void operator()(EndableT value) const {
            if (value.has_value()) {
              push(value.value());
            }
          }
        };

        return source(PushHandler{std::move(push)});
      }
    };
  }

  template <typename SourceT>
    requires concepts::Source<SourceT> && is_endable_v<source_value_t<SourceT>>
  RHEOSCAPE_CALLABLE auto unwrap_endable(SourceT source) {
    return detail::UnwrapEndableSourceBinder<SourceT>{std::move(source)};
  }

  namespace detail {
    struct UnwrapEndablePipeFactory {
      using is_pipe_factory = void;
      template <typename SourceT>
        requires concepts::Source<SourceT> && is_endable_v<source_value_t<SourceT>>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return unwrap_endable(std::move(source));
      }
    };
  }

  inline auto unwrap_endable() {
    return detail::UnwrapEndablePipeFactory{};
  }

}
