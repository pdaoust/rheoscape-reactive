#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <tuple>
#include <optional>
#include <logging.hpp>
#include <core_types.hpp>
#include <types/Wrapper.hpp>
#include <operators/map.hpp>
#include <util.hpp>

namespace rheo::operators {

  // combine streams together into one stream using a combining function.
  // If you're using this in a push stream, it won't start emitting values
  // until all sources have emitted a value.
  // Thereafter, it'll emit a combined value every time _any_ source emits a value,
  // updating the respective portion of the combined value.
  // Make sure all the streams being combined
  // will push a value every time they're pulled,
  // because it will only push a value once all streams have pushed.
  // If you're using it in a pull stream, it'll pull all sources and combine them.
  // An optional combiner function can be passed so you can combine the values the way you like;
  // the default just gloms them into a tuple.
  //
  // All of these sources end when any of their upstream sources ends.
  //
  // WORD OF CAUTION: If the first source is a source that pushes its first value on bind,
  // that first value will get missed because this operator needs to do some setup.

  namespace detail {
    // Helper to check if all optionals in a tuple have values
    template<typename... Ts>
    bool all_have_values(const std::tuple<std::optional<Ts>...>& values) {
      return std::apply([](const auto&... opts) {
        return (opts.has_value() && ...);
      }, values);
    }

    // Helper to reset all optionals in a tuple
    template<typename... Ts>
    void reset_all(std::tuple<std::optional<Ts>...>& values) {
      std::apply([](auto&... opts) {
        (opts.reset(), ...);
      }, values);
    }

    // Helper to extract and forward all values from optionals
    template<typename... Ts, size_t... Is>
    auto extract_values_impl(std::tuple<std::optional<Ts>...>& values, std::index_sequence<Is...>) {
      return std::make_tuple(std::forward<Ts>(std::get<Is>(values).value())...);
    }

    template<typename... Ts>
    auto extract_values(std::tuple<std::optional<Ts>...>& values) {
      return extract_values_impl(values, std::index_sequence_for<Ts...>{});
    }
  }

  // Variadic combine implementation
  // Usage: combine(combinerFn, source1, source2, ...)
  // Note: combiner is first to enable variadic parameter pack deduction
  template <typename CombineFn, typename T1, typename... Ts>
    requires (sizeof...(Ts) >= 1) && concepts::Combiner<CombineFn, T1, Ts...>
  RHEO_INLINE auto combine(
    CombineFn&& combiner,
    source_fn<T1> first,
    source_fn<Ts>... rest
  ) -> source_fn<std::invoke_result_t<CombineFn, T1, Ts...>> {
    using TOut = std::invoke_result_t<CombineFn, T1, Ts...>;
    constexpr size_t N = sizeof...(Ts) + 1;

    return [first, ... rest = std::move(rest), combiner = std::forward<CombineFn>(combiner)]
    (push_fn<TOut> push) mutable -> pull_fn {
      // Tuple to hold current values from each source
      auto currentValues = std::make_shared<std::tuple<std::optional<T1>, std::optional<Ts>...>>();

      // Tuple to hold pull functions: all are optional<pull_fn> for simplicity
      // Create one optional<pull_fn> for each source type
      auto pullFunctions = std::make_shared<std::tuple<
        decltype((void)std::declval<T1>(), std::optional<pull_fn>()),
        decltype((void)std::declval<Ts>(), std::optional<pull_fn>())...
      >>();

      // Bind each source with index-based handler
      [&]<size_t... Is>(std::index_sequence<Is...>) {
        // Create push handlers for each source using fold expression
        (
          [&]<size_t I>() {
            using TValue = std::tuple_element_t<I, std::tuple<T1, Ts...>>;

            // Push handler for source I
            auto pushHandler = [
              combiner,
              push,
              currentValues,
              pullFunctions
            ](TValue&& value) mutable {
              // Store the value
              std::get<I>(*currentValues).emplace(std::forward<TValue>(value));

              // Check if all values are ready
              if (detail::all_have_values(*currentValues)) {
                // All ready - push combined result
                auto extractedValues = detail::extract_values(*currentValues);
                auto result = std::apply(combiner, std::move(extractedValues));
                push(std::move(result));
                detail::reset_all(*currentValues);
              } else {
                // Not all ready - cascade pull to next source that needs a value
                [&]<size_t... Js>(std::index_sequence<Js...>) {
                  // Try to pull each subsequent source that doesn't have a value
                  (
                    [&]<size_t J>() {
                      if constexpr (J > I && J < N) {
                        // Check if source J needs a value and has a pull function
                        if (!std::get<J>(*currentValues).has_value()) {
                          if (std::get<J>(*pullFunctions).has_value()) {
                            std::get<J>(*pullFunctions).value()();
                          }
                          return true;  // Stop after pulling one
                        }
                      }
                      // Check sources before current if none after need values
                      if constexpr (J < I) {
                        if (!std::get<J>(*currentValues).has_value()) {
                          if (std::get<J>(*pullFunctions).has_value()) {
                            std::get<J>(*pullFunctions).value()();
                          }
                          return true;
                        }
                      }
                      return false;
                    }.template operator()<Js>() || ...
                  );
                }(std::make_index_sequence<N>{});
              }
            };

            // Bind the source to the push handler and store pull function
            auto sources_tuple = std::tie(first, rest...);
            std::get<I>(*pullFunctions).emplace(std::get<I>(sources_tuple)(pushHandler));
          }.template operator()<Is>(), ...
        );
      }(std::make_index_sequence<N>{});

      // Return pull function that pulls the first source
      return [pullFunctions]() {
        if (std::get<0>(*pullFunctions).has_value()) {
          std::get<0>(*pullFunctions).value()();
        }
      };
    };
  }

  // Pipe factory overload - requires explicitly specifying T1 template parameter.
  // Usage: source1 | combineWith<int>(combiner, source2, source3)
  //
  // NOTE: Unlike mergeWith() which can infer types from its source arguments,
  // combineWith() requires explicit T1 because pipe_fn<TOut, TIn> needs to know
  // TIn at the call site, but TIn (which is T1) cannot be deduced from the
  // combiner function alone - it could be any type the combiner accepts.
  template <typename T1, typename CombineFn, typename... Ts>
    requires concepts::Combiner<CombineFn, T1, Ts...>
  RHEO_INLINE auto combineWith(
    CombineFn&& combiner,
    source_fn<Ts>... sources
  ) -> pipe_fn<std::invoke_result_t<CombineFn, T1, Ts...>, T1> {
    using TOut = std::invoke_result_t<CombineFn, T1, Ts...>;
    return [combiner = std::forward<CombineFn>(combiner), ... sources = std::move(sources)]
    (source_fn<T1> source1) mutable -> source_fn<TOut> {
      return combine(std::move(combiner), std::move(source1), std::move(sources)...);
    };
  }

}
