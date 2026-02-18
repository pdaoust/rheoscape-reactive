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
  // To transform the tuple into a different type, use: combine(...) | map(mapper)
  //
  // All of these sources end when any of their upstream sources ends.
  //
  // WORD OF CAUTION: If the first source is a source that pushes its first value on bind,
  // that first value will get missed because this operator needs to do some setup.

  namespace detail {
    // Helper to check if all optionals in a tuple have values
    template<typename... Ts>
    RHEO_CALLABLE bool all_have_values(const std::tuple<std::optional<Ts>...>& values) {
      return std::apply([](const auto&... opts) {
        return (opts.has_value() && ...);
      }, values);
    }

    // Helper to reset all optionals in a tuple
    template<typename... Ts>
    RHEO_CALLABLE void reset_all(std::tuple<std::optional<Ts>...>& values) {
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
    RHEO_CALLABLE auto extract_values(std::tuple<std::optional<Ts>...>& values) {
      return extract_values_impl(values, std::index_sequence_for<Ts...>{});
    }

    // Cascade pull helper: pulls the next source that needs a value
    template<size_t I, size_t N, typename ValuesPtr, typename PullsPtr>
    RHEO_CALLABLE void cascade_pull(const ValuesPtr& current_values, const PullsPtr& pull_functions) {
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

    template <typename... SourceTs>
    struct CombineSourceBinder {
      using value_type = std::tuple<source_value_t<SourceTs>...>;
      static constexpr size_t N = sizeof...(SourceTs);

      std::tuple<SourceTs...> sources;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        using TOut = value_type;
        using ValuesType = std::tuple<std::optional<source_value_t<SourceTs>>...>;
        using PullsType = std::tuple<
          decltype((void)std::declval<source_value_t<SourceTs>>(), std::optional<pull_fn>())...
        >;

        auto current_values = std::make_shared<ValuesType>();
        auto pull_functions = std::make_shared<PullsType>();
        // Shared flag to track whether we're in a pull cascade.
        // All handlers share this so they know if the current push is from a cascade or spontaneous.
        auto in_cascade = std::make_shared<bool>(false);

        // Bind each source with its push handler.
        // Uses immediately-invoked template lambdas
        // because local classes can't have member templates.
        [&]<size_t... Is>(std::index_sequence<Is...>) {
          (
            [&]<size_t I>() {
              using TValue = std::tuple_element_t<I, std::tuple<source_value_t<SourceTs>...>>;

              struct PushHandler {
                PushFn push;
                std::shared_ptr<ValuesType> current_values;
                std::shared_ptr<PullsType> pull_functions;
                std::shared_ptr<bool> in_cascade;

                RHEO_CALLABLE void operator()(TValue value) const {
                  if (!*in_cascade) {
                    // This push is NOT from a pull cascade
                    // (e.g., spontaneous push from State.set()).
                    // Start a new cascade: reset all values to clear any stale data.
                    detail::reset_all(*current_values);
                    *in_cascade = true;
                  }

                  // Store the value at index I
                  std::get<I>(*current_values).emplace(std::move(value));

                  // Check if all values are ready
                  if (detail::all_have_values(*current_values)) {
                    // All ready - push combined result.
                    // Clear cascade flag BEFORE push
                    // to handle re-entrant pushes correctly.
                    *in_cascade = false;
                    auto result = detail::extract_values(*current_values);
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

              auto handler = PushHandler{
                push,
                current_values,
                pull_functions,
                in_cascade
              };

              std::get<I>(*pull_functions).emplace(std::get<I>(sources)(handler));
            }.template operator()<Is>(), ...
          );
        }(std::make_index_sequence<N>{});

        struct PullHandler {
          std::shared_ptr<PullsType> pull_functions;

          RHEO_CALLABLE void operator()() const {
            if (std::get<0>(*pull_functions).has_value()) {
              std::get<0>(*pull_functions).value()();
            }
          }
        };

        return PullHandler{pull_functions};
      }
    };
  }

  // Combine multiple sources into a tuple of their values.
  //
  // Usage: combine(source1, source2, source3)
  // To transform the tuple, use: combine(...) | map(mapper)
  template <typename... SourceTs>
    requires (sizeof...(SourceTs) >= 2) && (concepts::Source<SourceTs> && ...)
  RHEO_CALLABLE auto combine(SourceTs... sources) {
    return detail::CombineSourceBinder<SourceTs...>{
      std::make_tuple(std::move(sources)...)
    };
  }

  namespace detail {
    // Pipe factory for combine_with: combines the piped source with additional sources.
    template <typename... RestSourceTs>
    struct CombineWithPipeFactory {
      std::tuple<RestSourceTs...> sources;

      template <typename Source1T>
        requires concepts::Source<Source1T>
      RHEO_CALLABLE auto operator()(Source1T source1) const {
        return std::apply([&source1](auto... rest_sources) {
          return combine(std::move(source1), std::move(rest_sources)...);
        }, sources);
      }
    };
  }

  // Pipe factory: combine the piped source with additional sources into a tuple.
  // Usage: source1 | combine_with(source2, source3)
  template <typename... SourceTs>
    requires (concepts::Source<SourceTs> && ...)
  RHEO_CALLABLE auto combine_with(SourceTs... sources) {
    return detail::CombineWithPipeFactory<SourceTs...>{
      std::make_tuple(std::move(sources)...)
    };
  }

}
