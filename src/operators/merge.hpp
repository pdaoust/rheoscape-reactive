#pragma once

#include <functional>
#include <tuple>
#include <variant>
#include <core_types.hpp>
#include <operators/map.hpp>
#include <types/Wrapper.hpp>

// Merge streams together.
// If you pull a merged stream, all the upstream sources get pulled in sequence.
// If one stream has ended, it'll still keep pushing values from the others.

namespace rheo::operators {

  // Variadic merge for same-type sources - merges N sources of type T into one stream of type T
  // Usage: merge(source1, source2, source3, ...)
  template<typename T, typename... Sources>
    requires (sizeof...(Sources) >= 1) && (std::same_as<std::decay_t<Sources>, source_fn<T>> && ...)
  RHEO_INLINE source_fn<T> merge(source_fn<T> first, Sources&&... rest)
  {
    return [first, ... rest = std::forward<Sources>(rest)](push_fn<T> push) mutable {
      // Bind all sources to the same push function and collect their pull functions
      auto pullFns = std::make_tuple(first(push), (rest(push))...);

      // Return a pull function that pulls all sources in sequence
      return [pullFns]() {
        std::apply([](auto&&... pulls) {
          (pulls(), ...);  // Fold expression: call each pull function
        }, pullFns);
      };
    };
  }

  // Pipe factory for same-type merge
  // Usage: source1 | mergeWith(source2, source3, ...)
  template <typename T, typename... Sources>
    requires (std::same_as<std::decay_t<Sources>, source_fn<T>> && ...)
  RHEO_INLINE pipe_fn<T, T> mergeWith(Sources&&... sources)
  {
    return [... sources = std::forward<Sources>(sources)](source_fn<T> source1) mutable -> source_fn<T> {
      return merge(std::move(source1), std::forward<Sources>(sources)...);
    };
  }

  // Merge mixed-type sources into a variant stream
  // Example: mergeMixed(source_fn<int>, source_fn<float>) → source_fn<variant<int, float>>
  template<typename... Ts>
    requires (sizeof...(Ts) >= 2)
  RHEO_INLINE source_fn<std::variant<Ts...>> mergeMixed(source_fn<Ts>... sources) {
    return [... sources = std::move(sources)](push_fn<std::variant<Ts...>> push) mutable {
      // Wrap push function for each source to convert T -> variant<Ts...>
      auto pullFns = std::make_tuple((sources([push](auto&& value) {
        push(std::variant<Ts...>(std::forward<decltype(value)>(value)));
      }))...);

      // Return a pull function that pulls all sources in sequence
      return [pullFns]() {
        std::apply([](auto&&... pulls) {
          (pulls(), ...);
        }, pullFns);
      };
    };
  }

  // Pipe factory for mergeMixed
  // Usage: source1 | mergeWithMixed(source2, source3, ...)
  template <typename T1, typename... Ts>
    requires (sizeof...(Ts) >= 1)
  RHEO_INLINE pipe_fn<std::variant<T1, Ts...>, T1> mergeWithMixed(source_fn<Ts>... sources) {
    return [... sources = std::move(sources)](source_fn<T1> source1) mutable {
      return mergeMixed(std::move(source1), std::move(sources)...);
    };
  }

}