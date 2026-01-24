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

  // Combine multiple streams together into a single stream of tuples.
  //
  // When any source pushes a value, combine will:
  // 1. Reset all stored values (clearing any stale data from previous cascades)
  // 2. Store the pushed value
  // 3. Pull all other sources to get their current values
  // 4. If all sources responded to their pulls with pushes, emit a tuple of all values
  // 5. If any source has a no-op pull (didn't respond), no emission occurs
  //
  // This ensures that combine only emits when ALL sources provide fresh values
  // as a direct consequence of the current cascade. Spontaneous pushes from sources
  // (e.g., State.set() pushing to subscribers) will not cause emissions with stale
  // values from other sources.
  //
  // For pull-based usage, pulling combine will pull the first source, which triggers
  // the cascade described above.
  //
  // To transform the tuple into a different type, use: combine(...) | map_tuple(mapper)
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
    // Flag to track whether we're in a pull cascade.
    // Only values pushed during a cascade (as a direct result of pulling) should be used.
    std::shared_ptr<bool> in_cascade;

    RHEO_NOINLINE void operator()(TValue value) const {
      if (!*in_cascade) {
        // This push is NOT from a pull cascade (e.g., spontaneous push from State.set()).
        // Start a new cascade: reset all values to clear any stale data.
        detail::reset_all(*current_values);
        *in_cascade = true;
      }

      // Store the value at index I
      std::get<I>(*current_values).emplace(std::move(value));

      // Check if all values are ready
      if (detail::all_have_values(*current_values)) {
        // All ready - push combined result
        // Clear cascade flag BEFORE push to handle re-entrant pushes correctly
        *in_cascade = false;
        auto extracted_values = detail::extract_values(*current_values);
        auto result = std::apply(combiner, std::move(extracted_values));
        detail::reset_all(*current_values);
        push(std::move(result));
      } else {
        // Not all ready - cascade pull to next source that needs a value
        detail::cascade_pull<I, N>(current_values, pull_functions);
        // If we get here and still not all values, clear cascade flag
        // (the pulled source didn't respond with a push)
        if (!detail::all_have_values(*current_values)) {
          *in_cascade = false;
        }
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
      // Shared flag to track whether we're in a pull cascade.
      // All handlers share this so they know if the current push is from a cascade or spontaneous.
      auto in_cascade = std::make_shared<bool>(false);

      // Bind each source with its push handler
      bind_sources(push, current_values, pull_functions, in_cascade, std::make_index_sequence<N>{});

      return combine_pull_handler<T1, Ts...>{pull_functions};
    }

  private:
    template<size_t... Is>
    RHEO_NOINLINE void bind_sources(
      const push_fn<TOut>& push,
      const std::shared_ptr<ValuesType>& current_values,
      const std::shared_ptr<PullsType>& pull_functions,
      const std::shared_ptr<bool>& in_cascade,
      std::index_sequence<Is...>
    ) const {
      (bind_source<Is>(push, current_values, pull_functions, in_cascade), ...);
    }

    template<size_t I>
    RHEO_NOINLINE void bind_source(
      const push_fn<TOut>& push,
      const std::shared_ptr<ValuesType>& current_values,
      const std::shared_ptr<PullsType>& pull_functions,
      const std::shared_ptr<bool>& in_cascade
    ) const {
      using TValue = std::tuple_element_t<I, std::tuple<T1, Ts...>>;

      auto handler = combine_push_handler<I, N, CombineFn, TOut, T1, Ts...>{
        combiner,
        push,
        current_values,
        pull_functions,
        in_cascade
      };

      std::get<I>(*pull_functions).emplace(std::get<I>(sources)(handler));
    }
  };

  // Combine multiple sources into a tuple of their values.
  //
  // When any source pushes a value, combine will:
  // 1. Reset all stored values (clearing any stale data from previous cascades)
  // 2. Store the pushed value
  // 3. Pull all other sources to get their current values
  // 4. If all sources responded to their pulls with pushes, emit a tuple of all values
  // 5. If any source has a no-op pull (didn't respond), no emission occurs
  //
  // Usage: combine(source1, source2, source3)
  // To transform the tuple, use: combine(...) | map_tuple(mapper)
  template <typename T1, typename... Ts>
    requires (sizeof...(Ts) >= 1)
  RHEO_NOINLINE auto combine(
    source_fn<T1> first,
    source_fn<Ts>... rest
  ) -> source_fn<std::tuple<T1, Ts...>> {
    using TOut = std::tuple<T1, Ts...>;
    auto tuple_maker = [](T1 v1, Ts... vs) {
      return std::make_tuple(std::move(v1), std::move(vs)...);
    };
    using CombineFn = decltype(tuple_maker);

    return combine_source_binder<CombineFn, TOut, T1, Ts...>{
      std::move(tuple_maker),
      std::make_tuple(std::move(first), std::move(rest)...)
    };
  }

  // Pipe factory: combine the piped source with additional sources into a tuple.
  // Usage: source1 | combine_with<T1>(source2, source3)
  //
  // NOTE: Requires explicit T1 template parameter because pipe_fn needs to know
  // the input type at the call site before the source is piped in.
  template <typename T1, typename... Ts>
  RHEO_NOINLINE auto combine_with(
    source_fn<Ts>... sources
  ) -> pipe_fn<std::tuple<T1, Ts...>, T1> {
    return [... sources = std::move(sources)]
    (source_fn<T1> source1) mutable -> source_fn<std::tuple<T1, Ts...>> {
      return combine(std::move(source1), std::move(sources)...);
    };
  }

}
