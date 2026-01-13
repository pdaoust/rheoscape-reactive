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
    RHEO_NOINLINE bool all_have_values(const std::tuple<std::optional<Ts>...>& values) {
      return std::apply([](const auto&... opts) {
        return (opts.has_value() && ...);
      }, values);
    }

    // Helper to reset all optionals in a tuple
    template<typename... Ts>
    RHEO_NOINLINE void reset_all(std::tuple<std::optional<Ts>...>& values) {
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
    RHEO_NOINLINE auto extract_values(std::tuple<std::optional<Ts>...>& values) {
      return extract_values_impl(values, std::index_sequence_for<Ts...>{});
    }

    // Cascade pull helper: pulls the next source that needs a value
    template<size_t I, size_t N, typename ValuesPtr, typename PullsPtr>
    RHEO_NOINLINE void cascade_pull(const ValuesPtr& current_values, const PullsPtr& pull_functions) {
      // Try sources after I first, then sources before I
      [&]<size_t... Js>(std::index_sequence<Js...>) {
        (
          [&]<size_t J>() {
            if constexpr (J > I && J < N) {
              if (!std::get<J>(*current_values).has_value()) {
                if (std::get<J>(*pull_functions).has_value()) {
                  std::get<J>(*pull_functions).value()();
                }
                return true;
              }
            }
            if constexpr (J < I) {
              if (!std::get<J>(*current_values).has_value()) {
                if (std::get<J>(*pull_functions).has_value()) {
                  std::get<J>(*pull_functions).value()();
                }
                return true;
              }
            }
            return false;
          }.template operator()<Js>() || ...
        );
      }(std::make_index_sequence<N>{});
    }
  }

  // Named callable for combine push handler at index I
  template<size_t I, size_t N, typename CombineFn, typename TOut, typename... AllTypes>
  struct combine_push_handler {
    using TValue = std::tuple_element_t<I, std::tuple<AllTypes...>>;
    using ValuesType = std::tuple<std::optional<AllTypes>...>;
    using PullsType = std::tuple<decltype((void)std::declval<AllTypes>(), std::optional<pull_fn>())...>;

    CombineFn combiner;
    push_fn<TOut> push;
    std::shared_ptr<ValuesType> current_values;
    std::shared_ptr<PullsType> pull_functions;

    RHEO_NOINLINE void operator()(TValue value) const {
      // Store the value at index I
      std::get<I>(*current_values).emplace(std::move(value));

      // Check if all values are ready
      if (detail::all_have_values(*current_values)) {
        // All ready - push combined result
        auto extracted_values = detail::extract_values(*current_values);
        auto result = std::apply(combiner, std::move(extracted_values));
        push(std::move(result));
        detail::reset_all(*current_values);
      } else {
        // Not all ready - cascade pull to next source that needs a value
        detail::cascade_pull<I, N>(current_values, pull_functions);
      }
    }
  };

  // Named callable for combine pull handler
  template<typename... AllTypes>
  struct combine_pull_handler {
    using PullsType = std::tuple<decltype((void)std::declval<AllTypes>(), std::optional<pull_fn>())...>;

    std::shared_ptr<PullsType> pull_functions;

    RHEO_NOINLINE void operator()() const {
      if (std::get<0>(*pull_functions).has_value()) {
        std::get<0>(*pull_functions).value()();
      }
    }
  };

  // Named callable for combine source binder
  template<typename CombineFn, typename TOut, typename T1, typename... Ts>
  struct combine_source_binder {
    static constexpr size_t N = sizeof...(Ts) + 1;
    using ValuesType = std::tuple<std::optional<T1>, std::optional<Ts>...>;
    using PullsType = std::tuple<
      decltype((void)std::declval<T1>(), std::optional<pull_fn>()),
      decltype((void)std::declval<Ts>(), std::optional<pull_fn>())...
    >;

    CombineFn combiner;
    std::tuple<source_fn<T1>, source_fn<Ts>...> sources;

    RHEO_NOINLINE pull_fn operator()(push_fn<TOut> push) const {
      auto current_values = std::make_shared<ValuesType>();
      auto pull_functions = std::make_shared<PullsType>();

      // Bind each source with its push handler
      bind_sources(push, current_values, pull_functions, std::make_index_sequence<N>{});

      return combine_pull_handler<T1, Ts...>{pull_functions};
    }

  private:
    template<size_t... Is>
    RHEO_NOINLINE void bind_sources(
      const push_fn<TOut>& push,
      const std::shared_ptr<ValuesType>& current_values,
      const std::shared_ptr<PullsType>& pull_functions,
      std::index_sequence<Is...>
    ) const {
      (bind_source<Is>(push, current_values, pull_functions), ...);
    }

    template<size_t I>
    RHEO_NOINLINE void bind_source(
      const push_fn<TOut>& push,
      const std::shared_ptr<ValuesType>& current_values,
      const std::shared_ptr<PullsType>& pull_functions
    ) const {
      using TValue = std::tuple_element_t<I, std::tuple<T1, Ts...>>;

      auto handler = combine_push_handler<I, N, CombineFn, TOut, T1, Ts...>{
        combiner,
        push,
        current_values,
        pull_functions
      };

      std::get<I>(*pull_functions).emplace(std::get<I>(sources)(handler));
    }
  };

  // Variadic combine implementation
  // Usage: combine(combiner_fn, source1, source2, ...)
  // Note: combiner is first to enable variadic parameter pack deduction
  template <typename CombineFn, typename T1, typename... Ts>
    requires (sizeof...(Ts) >= 1) && concepts::Combiner<CombineFn, T1, Ts...>
  RHEO_NOINLINE auto combine(
    CombineFn combiner,
    source_fn<T1> first,
    source_fn<Ts>... rest
  ) -> source_fn<std::invoke_result_t<CombineFn, T1, Ts...>> {
    using TOut = std::invoke_result_t<CombineFn, T1, Ts...>;

    return combine_source_binder<CombineFn, TOut, T1, Ts...>{
      std::move(combiner),
      std::make_tuple(std::move(first), std::move(rest)...)
    };
  }

  // Pipe factory overload - requires explicitly specifying T1 template parameter.
  // Usage: source1 | combine_with<int>(combiner, source2, source3)
  //
  // NOTE: Unlike merge() which can infer types from its source arguments,
  // combine_with() requires explicit T1 because pipe_fn<TOut, TIn> needs to know
  // TIn at the call site, but TIn (which is T1) cannot be deduced from the
  // combiner function alone - it could be any type the combiner accepts.
  template <typename T1, typename CombineFn, typename... Ts>
    requires concepts::Combiner<CombineFn, T1, Ts...>
  RHEO_NOINLINE auto combine_with(
    CombineFn combiner,
    source_fn<Ts>... sources
  ) -> pipe_fn<std::invoke_result_t<CombineFn, T1, Ts...>, T1> {
    using TOut = std::invoke_result_t<CombineFn, T1, Ts...>;
    return [combiner = std::move(combiner), ... sources = std::move(sources)]
    (source_fn<T1> source1) mutable -> source_fn<TOut> {
      return combine(std::move(combiner), std::move(source1), std::move(sources)...);
    };
  }

}
