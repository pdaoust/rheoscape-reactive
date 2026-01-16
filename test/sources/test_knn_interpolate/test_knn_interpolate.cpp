#include <unity.h>
#include <cmath>
#include <sources/knn_interpolate.hpp>
#include <types/KnnStorage.hpp>
#include <types/State.hpp>

using namespace rheo;
using namespace rheo::sources;

// Simple 2D key for testing
struct Point2D {
  float x;
  float y;
};

// Euclidean distance function
float euclidean_distance(const Point2D& a, const Point2D& b) {
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

void test_knn_returns_nullopt_when_empty() {
  // Test that interpolation returns nullopt when storage is empty.
  KnnStorage<Point2D, float, float, 16> storage(euclidean_distance);

  State<Point2D> query_state(Point2D{1.0f, 1.0f});

  auto interp_source = knn_interpolate(
    query_state.get_source_fn(),
    storage,
    3  // k = 3 neighbors
  );

  std::optional<float> result;
  pull_fn pull = interp_source([&result](auto v) { result = v; });

  pull();

  TEST_ASSERT_FALSE_MESSAGE(result.has_value(),
    "Should return nullopt when storage is empty");
}

void test_knn_returns_exact_match() {
  // Test that exact match returns the stored value.
  KnnStorage<Point2D, float, float, 16> storage(euclidean_distance);

  storage.insert(Point2D{1.0f, 1.0f}, 10.0f);
  storage.insert(Point2D{2.0f, 2.0f}, 20.0f);
  storage.insert(Point2D{3.0f, 3.0f}, 30.0f);

  State<Point2D> query_state(Point2D{2.0f, 2.0f});

  auto interp_source = knn_interpolate(
    query_state.get_source_fn(),
    storage,
    1  // k = 1 (nearest only)
  );

  std::optional<float> result;
  pull_fn pull = interp_source([&result](auto v) { result = v; });

  pull();

  TEST_ASSERT_TRUE_MESSAGE(result.has_value(),
    "Should return a value for exact match");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 20.0f, result.value(),
    "Should return the exact stored value");
}

void test_knn_interpolates_between_neighbors() {
  // Test that interpolation averages values from neighbors.
  KnnStorage<Point2D, float, float, 16> storage(euclidean_distance);

  // Place 4 points at corners of a unit square
  storage.insert(Point2D{0.0f, 0.0f}, 10.0f);
  storage.insert(Point2D{2.0f, 0.0f}, 20.0f);
  storage.insert(Point2D{0.0f, 2.0f}, 30.0f);
  storage.insert(Point2D{2.0f, 2.0f}, 40.0f);

  // Query at the center
  State<Point2D> query_state(Point2D{1.0f, 1.0f});

  auto interp_source = knn_interpolate(
    query_state.get_source_fn(),
    storage,
    4  // k = 4 (all neighbors)
  );

  std::optional<float> result;
  pull_fn pull = interp_source([&result](auto v) { result = v; });

  pull();

  TEST_ASSERT_TRUE_MESSAGE(result.has_value(),
    "Should return an interpolated value");

  // All 4 corners are equidistant from center (distance = sqrt(2))
  // With inverse distance weighting and equal distances, should average to 25.0
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5f, 25.0f, result.value(),
    "Should average values of equidistant neighbors");
}

void test_knn_uses_custom_distance_fn() {
  // Test that custom distance function is used.
  // Use Manhattan distance instead of Euclidean
  auto manhattan = [](const Point2D& a, const Point2D& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
  };

  KnnStorage<Point2D, float, float, 16> storage(manhattan);

  storage.insert(Point2D{0.0f, 0.0f}, 10.0f);
  storage.insert(Point2D{1.0f, 1.0f}, 20.0f);  // Manhattan dist to (0.5, 0) = 1.5
  storage.insert(Point2D{2.0f, 0.0f}, 30.0f);  // Manhattan dist to (0.5, 0) = 1.5

  // With Manhattan distance, (1,1) and (2,0) are equidistant from (0.5, 0)
  State<Point2D> query_state(Point2D{0.5f, 0.0f});

  auto interp_source = knn_interpolate(
    query_state.get_source_fn(),
    storage,
    3
  );

  std::optional<float> result;
  pull_fn pull = interp_source([&result](auto v) { result = v; });

  pull();

  TEST_ASSERT_TRUE_MESSAGE(result.has_value(),
    "Should return value with custom distance function");
  // Result should be weighted average, closer point (0,0) should have more weight
}

void test_knn_uses_custom_weight_fn() {
  // Test that custom weight function is used.
  KnnStorage<Point2D, float, float, 16> storage(euclidean_distance);

  storage.insert(Point2D{0.0f, 0.0f}, 10.0f);
  storage.insert(Point2D{2.0f, 0.0f}, 20.0f);

  State<Point2D> query_state(Point2D{1.0f, 0.0f});  // Equidistant from both

  // Custom weight: all equal weight (ignores distance)
  auto uniform_weight = [](float distance) { return 1.0f; };

  auto interp_source = knn_interpolate(
    query_state.get_source_fn(),
    storage,
    2,
    uniform_weight
  );

  std::optional<float> result;
  pull_fn pull = interp_source([&result](auto v) { result = v; });

  pull();

  TEST_ASSERT_TRUE_MESSAGE(result.has_value(),
    "Should return value with custom weight function");
  // With uniform weights, should be simple average: (10 + 20) / 2 = 15
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 15.0f, result.value(),
    "Should compute uniform-weighted average");
}

void test_knn_uses_custom_combine_fn() {
  // Test that custom combine function is used.
  KnnStorage<Point2D, float, float, 16> storage(euclidean_distance);

  storage.insert(Point2D{0.0f, 0.0f}, 10.0f);
  storage.insert(Point2D{1.0f, 0.0f}, 20.0f);
  storage.insert(Point2D{2.0f, 0.0f}, 30.0f);

  // Query point is not an exact match with any stored point
  State<Point2D> query_state(Point2D{0.5f, 0.0f});

  // Custom combiner: return max value instead of weighted average
  auto max_combiner = [](const std::vector<std::pair<float, float>>& weighted_values) {
    float max_val = weighted_values[0].first;
    for (const auto& [value, weight] : weighted_values) {
      if (value > max_val) max_val = value;
    }
    return max_val;
  };

  auto interp_source = knn_interpolate(
    query_state.get_source_fn(),
    storage,
    3,
    inverse_distance_weight<float>,
    max_combiner
  );

  std::optional<float> result;
  pull_fn pull = interp_source([&result](auto v) { result = v; });

  pull();

  TEST_ASSERT_TRUE_MESSAGE(result.has_value(),
    "Should return value with custom combine function");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 30.0f, result.value(),
    "Should return max value from custom combiner");
}

void test_knn_respects_k_parameter() {
  // Test that only k nearest neighbors are used.
  KnnStorage<Point2D, float, float, 16> storage(euclidean_distance);

  // Add 5 points with increasing distance from origin
  storage.insert(Point2D{0.0f, 0.0f}, 100.0f);   // distance = 0
  storage.insert(Point2D{1.0f, 0.0f}, 10.0f);    // distance = 1
  storage.insert(Point2D{2.0f, 0.0f}, 20.0f);    // distance = 2
  storage.insert(Point2D{10.0f, 0.0f}, 1000.0f); // distance = 10
  storage.insert(Point2D{100.0f, 0.0f}, 10000.0f); // distance = 100

  State<Point2D> query_state(Point2D{0.0f, 0.0f});

  // With k=1, should only get the exact match at origin
  auto interp_source_k1 = knn_interpolate(
    query_state.get_source_fn(),
    storage,
    1
  );

  std::optional<float> result_k1;
  pull_fn pull_k1 = interp_source_k1([&result_k1](auto v) { result_k1 = v; });
  pull_k1();

  TEST_ASSERT_TRUE_MESSAGE(result_k1.has_value(), "Should return value for k=1");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 100.0f, result_k1.value(),
    "k=1 should return nearest value only");

  // With k=2, should get weighted average of origin and (1,0)
  auto interp_source_k2 = knn_interpolate(
    query_state.get_source_fn(),
    storage,
    2
  );

  std::optional<float> result_k2;
  pull_fn pull_k2 = interp_source_k2([&result_k2](auto v) { result_k2 = v; });
  pull_k2();

  TEST_ASSERT_TRUE_MESSAGE(result_k2.has_value(), "Should return value for k=2");
  // Exact match at origin has distance 0, so very high weight
  // The result should be close to 100 but slightly influenced by 10
  TEST_ASSERT_TRUE_MESSAGE(result_k2.value() > 50.0f,
    "k=2 should heavily weight the exact match");
}

void test_knn_storage_find_k_nearest() {
  // Direct test of KnnStorage::find_k_nearest.
  KnnStorage<Point2D, float, float, 16> storage(euclidean_distance);

  storage.insert(Point2D{0.0f, 0.0f}, 10.0f);
  storage.insert(Point2D{3.0f, 0.0f}, 30.0f);
  storage.insert(Point2D{1.0f, 0.0f}, 15.0f);
  storage.insert(Point2D{2.0f, 0.0f}, 20.0f);

  Point2D query{0.0f, 0.0f};
  auto neighbors = storage.find_k_nearest(query, 3);

  TEST_ASSERT_EQUAL_MESSAGE(3, neighbors.size(),
    "Should return exactly k neighbors");

  // Neighbors should be sorted by distance
  // (0,0) distance 0, (1,0) distance 1, (2,0) distance 2
  float prev_distance = -1.0f;
  for (const auto& [key, value, distance] : neighbors) {
    TEST_ASSERT_TRUE_MESSAGE(distance >= prev_distance,
      "Neighbors should be sorted by distance");
    prev_distance = distance;
  }

  // First neighbor should be the exact match
  auto& [first_key, first_value, first_dist] = neighbors[0];
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 10.0f, first_value,
    "First neighbor should be the closest point");
}

void test_knn_storage_insert_and_count() {
  // Test basic insert and count operations.
  KnnStorage<Point2D, float, float, 4> storage(euclidean_distance);

  TEST_ASSERT_EQUAL_MESSAGE(0, storage.count(), "Initial count should be 0");

  storage.insert(Point2D{1.0f, 1.0f}, 10.0f);
  TEST_ASSERT_EQUAL_MESSAGE(1, storage.count(), "Count should be 1 after insert");

  storage.insert(Point2D{2.0f, 2.0f}, 20.0f);
  storage.insert(Point2D{3.0f, 3.0f}, 30.0f);
  TEST_ASSERT_EQUAL_MESSAGE(3, storage.count(), "Count should be 3");

  // Try to insert when full (capacity = 4)
  storage.insert(Point2D{4.0f, 4.0f}, 40.0f);
  TEST_ASSERT_EQUAL_MESSAGE(4, storage.count(), "Count should be 4 (full)");

  // This should fail (storage full)
  bool inserted = storage.insert(Point2D{5.0f, 5.0f}, 50.0f);
  TEST_ASSERT_FALSE_MESSAGE(inserted, "Insert should fail when storage is full");
  TEST_ASSERT_EQUAL_MESSAGE(4, storage.count(), "Count should still be 4");
}

void test_knn_storage_insert_with_eviction() {
  // Test insert with eviction when storage is full.
  KnnStorage<Point2D, float, float, 3> storage(euclidean_distance);

  storage.insert(Point2D{0.0f, 0.0f}, 10.0f);
  storage.insert(Point2D{1.0f, 0.0f}, 20.0f);
  storage.insert(Point2D{2.0f, 0.0f}, 30.0f);
  TEST_ASSERT_EQUAL_MESSAGE(3, storage.count(), "Storage should be full");

  // Insert with eviction - new point at (100, 0)
  // This should evict the point farthest from (100, 0), which is (0, 0)
  storage.insert_with_eviction(Point2D{100.0f, 0.0f}, 100.0f);
  TEST_ASSERT_EQUAL_MESSAGE(3, storage.count(), "Count should still be 3");

  // Check that (0, 0) was evicted by looking for it
  auto result = storage.find_exact(Point2D{0.0f, 0.0f}, 0.01f);
  TEST_ASSERT_FALSE_MESSAGE(result.has_value(),
    "Point (0,0) should have been evicted");

  // Check that (100, 0) was inserted
  auto new_result = storage.find_exact(Point2D{100.0f, 0.0f}, 0.01f);
  TEST_ASSERT_TRUE_MESSAGE(new_result.has_value(),
    "Point (100,0) should have been inserted");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 100.0f, new_result.value(),
    "Value at (100,0) should be 100");
}

void test_knn_storage_remove() {
  // Test remove operation.
  KnnStorage<Point2D, float, float, 16> storage(euclidean_distance);

  storage.insert(Point2D{1.0f, 1.0f}, 10.0f);
  storage.insert(Point2D{2.0f, 2.0f}, 20.0f);
  TEST_ASSERT_EQUAL_MESSAGE(2, storage.count(), "Count should be 2");

  bool removed = storage.remove(Point2D{1.0f, 1.0f}, 0.01f);
  TEST_ASSERT_TRUE_MESSAGE(removed, "Remove should succeed");
  TEST_ASSERT_EQUAL_MESSAGE(1, storage.count(), "Count should be 1 after remove");

  // Try to remove again (should fail)
  removed = storage.remove(Point2D{1.0f, 1.0f}, 0.01f);
  TEST_ASSERT_FALSE_MESSAGE(removed, "Second remove should fail");
}

void test_knn_storage_clear() {
  // Test clear operation.
  KnnStorage<Point2D, float, float, 16> storage(euclidean_distance);

  storage.insert(Point2D{1.0f, 1.0f}, 10.0f);
  storage.insert(Point2D{2.0f, 2.0f}, 20.0f);
  storage.insert(Point2D{3.0f, 3.0f}, 30.0f);
  TEST_ASSERT_EQUAL_MESSAGE(3, storage.count(), "Count should be 3");

  storage.clear();
  TEST_ASSERT_EQUAL_MESSAGE(0, storage.count(), "Count should be 0 after clear");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_knn_returns_nullopt_when_empty);
  RUN_TEST(test_knn_returns_exact_match);
  RUN_TEST(test_knn_interpolates_between_neighbors);
  RUN_TEST(test_knn_uses_custom_distance_fn);
  RUN_TEST(test_knn_uses_custom_weight_fn);
  RUN_TEST(test_knn_uses_custom_combine_fn);
  RUN_TEST(test_knn_respects_k_parameter);
  RUN_TEST(test_knn_storage_find_k_nearest);
  RUN_TEST(test_knn_storage_insert_and_count);
  RUN_TEST(test_knn_storage_insert_with_eviction);
  RUN_TEST(test_knn_storage_remove);
  RUN_TEST(test_knn_storage_clear);
  UNITY_END();
}
