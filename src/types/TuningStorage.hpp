#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <limits>
#include <operators/pid_autotune/autotune_types.hpp>

namespace rheoscape {

  // Fixed-size storage buffer for tuning records
  // Designed to be platform-agnostic - caller allocates storage
  // Can be placed in NVRAM, EEPROM, or regular RAM
  template <typename TRecord, size_t N>
  struct TuningStorage {
    TRecord records[N];
    size_t count;
    uint32_t version;  // Incremented on write (for cache invalidation)

    // Initialize all slots as invalid
    void initialize() {
      count = 0;
      version = 0;
      for (size_t i = 0; i < N; ++i) {
        records[i].valid = false;
      }
    }

    // Find a record by index
    std::optional<TRecord> get(size_t index) const {
      if (index < N && records[index].valid) {
        return records[index];
      }
      return std::nullopt;
    }

    // Get all valid records count
    size_t valid_count() const {
      size_t valid = 0;
      for (size_t i = 0; i < N; ++i) {
        if (records[i].valid) {
          ++valid;
        }
      }
      return valid;
    }

    // Find first empty slot, or return nullopt if full
    std::optional<size_t> find_empty_slot() const {
      for (size_t i = 0; i < N; ++i) {
        if (!records[i].valid) {
          return i;
        }
      }
      return std::nullopt;
    }

    // Find slot with lowest quality (highest score = worst)
    std::optional<size_t> find_lowest_quality_slot() const {
      std::optional<size_t> worst_idx = std::nullopt;
      float worst_score = -std::numeric_limits<float>::infinity();

      for (size_t i = 0; i < N; ++i) {
        if (records[i].valid && records[i].quality_score > worst_score) {
          worst_score = records[i].quality_score;
          worst_idx = i;
        }
      }
      return worst_idx;
    }

    // Find oldest slot by timestamp (for LRU eviction)
    std::optional<size_t> find_oldest_slot() const {
      std::optional<size_t> oldest_idx = std::nullopt;
      uint32_t oldest_time = std::numeric_limits<uint32_t>::max();

      for (size_t i = 0; i < N; ++i) {
        if (records[i].valid && records[i].timestamp < oldest_time) {
          oldest_time = records[i].timestamp;
          oldest_idx = i;
        }
      }
      return oldest_idx;
    }

    // Write a record to a specific slot
    void write(size_t index, const TRecord& record) {
      if (index < N) {
        bool was_valid = records[index].valid;
        records[index] = record;
        records[index].valid = true;
        if (!was_valid) {
          ++count;
        }
        ++version;
      }
    }

    // Invalidate a slot
    void invalidate(size_t index) {
      if (index < N && records[index].valid) {
        records[index].valid = false;
        --count;
        ++version;
      }
    }

    // Clear all records
    void clear() {
      initialize();
    }
  };

  // Helper to create storage with proper initialization
  // Usage: auto storage = make_tuning_storage<TRecord, N>();
  template <typename TRecord, size_t N>
  TuningStorage<TRecord, N> make_tuning_storage() {
    TuningStorage<TRecord, N> storage;
    storage.initialize();
    return storage;
  }

  // Utility functions for working with TuningStorage

  // Find record matching plant params within distance threshold
  // Returns index of matching record, or nullopt if no match
  template <typename TRecord, size_t N, typename TPlantParams>
  std::optional<size_t> find_matching_record(
    const TuningStorage<TRecord, N>& storage,
    const TPlantParams& params,
    float distance_threshold
  ) {
    for (size_t i = 0; i < N; ++i) {
      if (storage.records[i].valid) {
        auto dist = params.distance_to(storage.records[i].plant_params);
        if (dist <= distance_threshold) {
          return i;
        }
      }
    }
    return std::nullopt;
  }

  // Find k nearest neighbors to given plant params
  // Returns vector of (index, distance) pairs sorted by distance
  template <typename TRecord, size_t N, typename TPlantParams, size_t K>
  size_t find_k_nearest(
    const TuningStorage<TRecord, N>& storage,
    const TPlantParams& params,
    size_t k,
    size_t (&out_indices)[K],
    float (&out_distances)[K]
  ) {
    // Initialize output arrays
    for (size_t i = 0; i < K; ++i) {
      out_indices[i] = N;  // Invalid index
      out_distances[i] = std::numeric_limits<float>::infinity();
    }

    size_t found = 0;

    // For each valid record, check if it's closer than current k-nearest
    for (size_t i = 0; i < N; ++i) {
      if (!storage.records[i].valid) continue;

      float dist = params.distance_to(storage.records[i].plant_params);

      // Find position to insert (maintain sorted order)
      size_t insert_pos = found < k ? found : k;
      for (size_t j = 0; j < (found < k ? found : k); ++j) {
        if (dist < out_distances[j]) {
          insert_pos = j;
          break;
        }
      }

      // If this record is closer than the k-th nearest, insert it
      if (insert_pos < k) {
        // Shift elements to make room
        for (size_t j = k - 1; j > insert_pos; --j) {
          out_indices[j] = out_indices[j - 1];
          out_distances[j] = out_distances[j - 1];
        }
        out_indices[insert_pos] = i;
        out_distances[insert_pos] = dist;
        if (found < k) ++found;
      }
    }

    return found;
  }

}
