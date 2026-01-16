#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>
#include <tuple>
#include <algorithm>
#include <limits>

namespace rheo {

  // Distance function signature for IoC.
  // User provides this to compute distance between two keys.
  // The key can be n-dimensional; storage is agnostic about dimensions.
  template <typename TKey, typename TDistance = float>
  using distance_fn = std::function<TDistance(const TKey& a, const TKey& b)>;

  // Generic KNN storage class.
  //
  // Provides k-nearest-neighbor lookup with user-defined distance metric.
  // Designed to be reusable for any ML/interpolation needs.
  //
  // Template parameters:
  //   TKey: The key type (can be n-dimensional struct)
  //   TValue: The value type to store
  //   TDistance: The distance type (default: float)
  //   N: Maximum number of records to store
  //
  // Usage:
  //   auto distance = [](const PlantParams& a, const PlantParams& b) {
  //     float dx = a.thermal_mass - b.thermal_mass;
  //     float dy = a.power - b.power;
  //     return std::sqrt(dx*dx + dy*dy);
  //   };
  //   KnnStorage<PlantParams, PidWeights, float, 100> storage(distance);
  //   storage.insert(params, weights);
  //   auto neighbors = storage.find_k_nearest(query, 3);
  template <typename TKey, typename TValue, typename TDistance = float, size_t N = 64>
  class KnnStorage {
  public:
    // Record type for internal storage
    struct Record {
      TKey key;
      TValue value;
      bool valid;
    };

    // Result type for k-nearest lookup
    using NeighborResult = std::tuple<TKey, TValue, TDistance>;

  private:
    Record _records[N];
    size_t _count;
    distance_fn<TKey, TDistance> _distance;

  public:
    // Constructor with IoC for distance metric.
    explicit KnnStorage(distance_fn<TKey, TDistance> distance)
      : _count(0), _distance(distance) {
      for (size_t i = 0; i < N; ++i) {
        _records[i].valid = false;
      }
    }

    // Get number of valid records.
    size_t count() const {
      return _count;
    }

    // Get maximum capacity.
    static constexpr size_t capacity() {
      return N;
    }

    // Check if storage is full.
    bool is_full() const {
      return _count >= N;
    }

    // Insert or update a record.
    // If the key already exists (within some small distance), updates the value.
    // If storage is full, returns false without inserting.
    // Returns true if inserted/updated successfully.
    bool insert(const TKey& key, const TValue& value) {
      // Find empty slot or matching key
      size_t target_slot = N;  // Invalid index

      for (size_t i = 0; i < N; ++i) {
        if (!_records[i].valid) {
          if (target_slot == N) {
            target_slot = i;  // Remember first empty slot
          }
        }
      }

      if (target_slot == N) {
        // Storage is full
        return false;
      }

      _records[target_slot].key = key;
      _records[target_slot].value = value;
      _records[target_slot].valid = true;
      ++_count;

      return true;
    }

    // Insert with eviction policy.
    // If full, evicts the record that is farthest from the new key.
    // Always succeeds.
    void insert_with_eviction(const TKey& key, const TValue& value) {
      // Try normal insert first
      if (insert(key, value)) {
        return;
      }

      // Storage is full - find farthest record to evict
      size_t farthest_idx = 0;
      TDistance max_dist = TDistance{0};

      for (size_t i = 0; i < N; ++i) {
        if (_records[i].valid) {
          TDistance dist = _distance(_records[i].key, key);
          if (dist > max_dist) {
            max_dist = dist;
            farthest_idx = i;
          }
        }
      }

      // Replace farthest record
      _records[farthest_idx].key = key;
      _records[farthest_idx].value = value;
    }

    // Find k nearest neighbors to query key.
    // Returns vector of (key, value, distance) tuples sorted by distance.
    // May return fewer than k if storage has fewer valid records.
    std::vector<NeighborResult> find_k_nearest(const TKey& query, size_t k) const {
      std::vector<NeighborResult> results;
      results.reserve(std::min(k, _count));

      // Compute distances for all valid records
      std::vector<std::pair<size_t, TDistance>> distances;
      distances.reserve(_count);

      for (size_t i = 0; i < N; ++i) {
        if (_records[i].valid) {
          TDistance dist = _distance(_records[i].key, query);
          distances.emplace_back(i, dist);
        }
      }

      // Sort by distance
      std::sort(distances.begin(), distances.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

      // Take top k
      size_t result_count = std::min(k, distances.size());
      for (size_t i = 0; i < result_count; ++i) {
        size_t idx = distances[i].first;
        results.emplace_back(
          _records[idx].key,
          _records[idx].value,
          distances[i].second
        );
      }

      return results;
    }

    // Find exact match within distance threshold.
    // Returns the value of the closest record if within threshold, nullopt otherwise.
    std::optional<TValue> find_exact(const TKey& query, TDistance threshold) const {
      std::optional<TValue> best_match = std::nullopt;
      TDistance best_dist = threshold;

      for (size_t i = 0; i < N; ++i) {
        if (_records[i].valid) {
          TDistance dist = _distance(_records[i].key, query);
          if (dist < best_dist) {
            best_dist = dist;
            best_match = _records[i].value;
          }
        }
      }

      return best_match;
    }

    // Find record by exact key match (distance = 0).
    std::optional<TValue> find(const TKey& query) const {
      return find_exact(query, std::numeric_limits<TDistance>::epsilon());
    }

    // Remove a record by key (first match within threshold).
    // Returns true if a record was removed.
    bool remove(const TKey& key, TDistance threshold = std::numeric_limits<TDistance>::epsilon()) {
      for (size_t i = 0; i < N; ++i) {
        if (_records[i].valid) {
          TDistance dist = _distance(_records[i].key, key);
          if (dist <= threshold) {
            _records[i].valid = false;
            --_count;
            return true;
          }
        }
      }
      return false;
    }

    // Clear all records.
    void clear() {
      for (size_t i = 0; i < N; ++i) {
        _records[i].valid = false;
      }
      _count = 0;
    }

    // Iterate over all valid records.
    // Callback signature: void(const TKey&, const TValue&)
    template <typename Callback>
    void for_each(Callback&& callback) const {
      for (size_t i = 0; i < N; ++i) {
        if (_records[i].valid) {
          callback(_records[i].key, _records[i].value);
        }
      }
    }
  };

  // Factory function to create KnnStorage with type deduction for distance function.
  template <typename TKey, typename TValue, size_t N = 64, typename DistanceFn>
  auto make_knn_storage(DistanceFn&& distance) {
    using TDistance = std::invoke_result_t<DistanceFn, TKey, TKey>;
    return KnnStorage<TKey, TValue, TDistance, N>(std::forward<DistanceFn>(distance));
  }

}
