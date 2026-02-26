#pragma once

#include <functional>
#include <chrono>
#include <types/core_types.hpp>
#include <types/Range.hpp>
#include <types/au_all_units_noio.hpp>
#include <operators/combine.hpp>
#include <operators/map.hpp>

namespace rheoscape::operators {

  namespace detail {
    // Helper to convert a value to TOut, using appropriate conversion for:
    // - au types (rep_cast)
    // - std::chrono types (duration_cast / time_point_cast)
    // - other types (static_cast)
    template<typename TOut, typename TFrom>
    TOut convert_to(TFrom&& value) {
      if constexpr (requires { au::rep_cast<typename TOut::Rep>(value); }) {
        // Use au's rep_cast for au types (Quantity, QuantityPoint).
        return au::rep_cast<typename TOut::Rep>(value);
      } else if constexpr (is_chrono_duration_v<TOut>) {
        // Use duration_cast for std::chrono::duration.
        return std::chrono::duration_cast<TOut>(value);
      } else if constexpr (is_chrono_time_point_v<TOut>) {
        // Use time_point_cast for std::chrono::time_point.
        return std::chrono::time_point_cast<typename TOut::duration>(value);
      } else {
        // Fall back to static_cast for other types.
        return static_cast<TOut>(std::forward<TFrom>(value));
      }
    }

  }

  template <typename SourceT, typename FromSourceT, typename ToSourceT>
    requires concepts::Source<SourceT> && concepts::Source<FromSourceT> && concepts::Source<ToSourceT>
  RHEOSCAPE_CALLABLE auto normalize(SourceT source, FromSourceT from_source, ToSourceT to_source) {
    using TIn = source_value_t<SourceT>;
    using TOut = typename source_value_t<ToSourceT>::value_type;

    // Named callable for normalize's combiner function - provides better stack traces in debug mode.
    // Note: The input type (TIn) is used in division during interpolation, so ensure TIn has
    // sufficient precision for your needs (e.g., use float rather than int to avoid truncation).
    struct Combiner {
      RHEOSCAPE_CALLABLE TOut operator()(TIn value, Range<TIn> from, Range<TOut> to) const {
        if (value <= from.min) {
          return to.min;
        } else if (value >= from.max) {
          return to.max;
        } else {
          // The arithmetic may produce a different type due to promotion (e.g., float from int division).
          // Use convert_to to handle type conversion appropriately for both au and non-au types.
          auto result = to.min + (to.max - to.min) * (value - from.min) / (from.max - from.min);
          return detail::convert_to<TOut>(result);
        }
      }
    };

    return combine(std::move(source), std::move(from_source), std::move(to_source))
      | map(Combiner{});
  }

  namespace detail {
    template <typename FromSourceT, typename ToSourceT>
    struct NormalizePipeFactory {
      using is_pipe_factory = void;
      using TOut = typename source_value_t<ToSourceT>::value_type;
      FromSourceT from_source;
      ToSourceT to_source;

      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return normalize(std::move(source), FromSourceT(from_source), ToSourceT(to_source));
      }
    };
  }

  template <typename FromSourceT, typename ToSourceT>
    requires concepts::Source<FromSourceT> && concepts::Source<ToSourceT>
  auto normalize(FromSourceT from_source, ToSourceT to_source) {
    return detail::NormalizePipeFactory<FromSourceT, ToSourceT>{
      std::move(from_source), std::move(to_source)
    };
  }

}
