#pragma once

#include <optional>
#include <vector>
#include <functional>
#include <limits>
#include <types/core_types.hpp>
#include <operators/map.hpp>
#include <types/KnnStorage.hpp>

namespace rheoscape::sources {

  // Weight function signature for IoC.
  // User provides this to compute interpolation weight from distance.
  // Default: inverse distance weighting.
  template <typename TDistance = float>
  using weight_fn = std::function<TDistance(TDistance distance)>;

  // Value combiner function signature for IoC.
  // User provides this to combine weighted values into a single result.
  template <typename TValue, typename TWeight = float>
  using combine_fn = std::function<TValue(
    const std::vector<std::pair<TValue, TWeight>>& weighted_values
  )>;

  // Default inverse distance weighting.
  // Returns 1/d for d > 0, or a large value for d = 0.
  template <typename TDistance>
  TDistance inverse_distance_weight(TDistance d) {
    constexpr TDistance epsilon = TDistance(0.0001);
    if (d < epsilon) {
      return std::numeric_limits<TDistance>::max() / TDistance(2);
    }
    return TDistance(1) / d;
  }

  // Default weighted average combiner for scalar types.
  // Computes sum(value * weight) / sum(weight).
  template <typename TValue, typename TWeight = float>
  TValue weighted_average_scalar(
    const std::vector<std::pair<TValue, TWeight>>& weighted_values
  ) {
    if (weighted_values.empty()) {
      return TValue{};
    }

    TWeight total_weight = TWeight{0};
    TValue weighted_sum = TValue{0};

    for (const auto& [value, weight] : weighted_values) {
      weighted_sum = weighted_sum + value * static_cast<TValue>(weight);
      total_weight = total_weight + weight;
    }

    if (total_weight < TWeight(0.0001)) {
      // All weights zero, return first value
      return weighted_values[0].first;
    }

    return weighted_sum / static_cast<TValue>(total_weight);
  }

  // Named callable for KNN interpolation.
  template <
    typename TKey,
    typename TValue,
    typename TDistance,
    size_t N,
    typename WeightFn,
    typename CombineFn
  >
  struct knn_interpolate_mapper {
    const KnnStorage<TKey, TValue, TDistance, N>* storage;
    size_t k;
    WeightFn weight_fn;
    CombineFn combine_fn;

    RHEOSCAPE_CALLABLE std::optional<TValue> operator()(TKey key) const {
      auto neighbors = storage->find_k_nearest(key, k);

      if (neighbors.empty()) {
        return std::nullopt;
      }

      // Check for exact match (distance ≈ 0) to avoid overflow issues
      constexpr TDistance epsilon = TDistance(0.0001);
      const auto& [first_key, first_value, first_distance] = neighbors[0];
      if (first_distance < epsilon) {
        // Exact match - return value directly
        return first_value;
      }

      // Compute weights and build weighted values vector
      std::vector<std::pair<TValue, TDistance>> weighted_values;
      weighted_values.reserve(neighbors.size());

      for (const auto& [neighbor_key, neighbor_value, distance] : neighbors) {
        TDistance weight = weight_fn(distance);
        weighted_values.emplace_back(neighbor_value, weight);
      }

      // Combine using user-provided combiner
      return combine_fn(weighted_values);
    }
  };

  // KNN interpolate source function factory.
  //
  // Creates a source that looks up values by interpolating from k-nearest
  // neighbors in storage. Uses inverse-distance-weighted averaging by default.
  //
  // Template parameters:
  //   TKey: Key type for lookup
  //   TValue: Value type in storage
  //   TDistance: Distance metric type
  //   N: Storage capacity
  //
  // Parameters:
  //   key_source: Source providing keys to look up
  //   storage: KnnStorage to search
  //   k: Number of nearest neighbors to use
  //   weight: Function to convert distance to interpolation weight (default: 1/d)
  //   combine: Function to combine weighted values (default: weighted average)
  //
  // Returns:
  //   A source of std::optional<TValue> - nullopt if storage is empty
  //
  // Usage:
  //   auto interpolated = knn_interpolate(
  //     plant_params_source,
  //     storage,
  //     3  // Use 3 nearest neighbors
  //   );
  template <
    typename KeySourceFn,
    typename TKey,
    typename TValue,
    typename TDistance,
    size_t N,
    typename WeightFn = weight_fn<TDistance>,
    typename CombineFn = combine_fn<TValue, TDistance>
  >
    requires concepts::SourceOf<KeySourceFn, TKey>
  auto knn_interpolate(
    KeySourceFn key_source,
    const KnnStorage<TKey, TValue, TDistance, N>& storage,
    size_t k,
    WeightFn weight = inverse_distance_weight<TDistance>,
    CombineFn combine = weighted_average_scalar<TValue, TDistance>
  ) {
    using Mapper = knn_interpolate_mapper<TKey, TValue, TDistance, N, WeightFn, CombineFn>;

    return operators::map(
      std::move(key_source),
      Mapper{&storage, k, weight, combine}
    );
  }

  namespace detail {

    // Pipe factory for knn_interpolate.
    template <
      typename TKey,
      typename TValue,
      typename TDistance,
      size_t N,
      typename WeightFn,
      typename CombineFn
    >
    struct knn_interpolate_pipe_factory {
      const KnnStorage<TKey, TValue, TDistance, N>* storage;
      size_t k;
      WeightFn weight;
      CombineFn combine;

      template <typename KeySourceFn>
        requires concepts::SourceOf<KeySourceFn, TKey>
      auto operator()(KeySourceFn key_source) const {
        return knn_interpolate<TValue, TDistance, N>(
          std::move(key_source), *storage, k, weight, combine
        );
      }
    };

  } // namespace detail

  // Pipe version of knn_interpolate.
  template <
    typename TKey,
    typename TValue,
    typename TDistance = float,
    size_t N = 64,
    typename WeightFn = weight_fn<TDistance>,
    typename CombineFn = combine_fn<TValue, TDistance>
  >
  auto knn_interpolate(
    const KnnStorage<TKey, TValue, TDistance, N>& storage,
    size_t k,
    WeightFn weight = inverse_distance_weight<TDistance>,
    CombineFn combine = weighted_average_scalar<TValue, TDistance>
  ) {
    return detail::knn_interpolate_pipe_factory<TKey, TValue, TDistance, N, WeightFn, CombineFn>{
      &storage, k, weight, combine
    };
  }

}
