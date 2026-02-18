#include <unity.h>
#include <string>
#include <map>
#include <vector>
#include <sinks/table_sink.hpp>
#include <types/State.hpp>

using namespace rheoscape;
using namespace rheoscape::sinks;

void test_table_sink_calls_store_fn() {
  // Test that table_sink calls the injected store function.
  std::vector<std::pair<std::string, int>> stored_items;

  auto store = [&stored_items](const std::string& key, const int& value) {
    stored_items.emplace_back(key, value);
    return true;
  };

  State<TableSinkInput<std::string, int>> input_state(
    make_table_sink_input(std::string("key1"), 42)
  );

  auto sink = table_sink<std::string, int>(store);
  // Use initial_push=false to avoid double-push (State would push on bind + pull)
  sink(input_state.get_source_fn(false));

  TEST_ASSERT_EQUAL_MESSAGE(1, stored_items.size(),
    "Store function should be called once");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("key1", stored_items[0].first.c_str(),
    "Key should be correct");
  TEST_ASSERT_EQUAL_MESSAGE(42, stored_items[0].second,
    "Value should be correct");
}

void test_table_sink_pullable_returns_pull_fn() {
  // Test that table_sink_pullable returns a usable pull function.
  std::vector<std::pair<std::string, int>> stored_items;

  auto store = [&stored_items](const std::string& key, const int& value) {
    stored_items.emplace_back(key, value);
    return true;
  };

  State<TableSinkInput<std::string, int>> input_state(
    make_table_sink_input(std::string("initial"), 0)
  );

  auto sink = table_sink_pullable<std::string, int>(store);
  // Use initial_push=false to avoid auto-push on bind
  pull_fn pull = sink(input_state.get_source_fn(false));

  // Initially nothing stored (pullable doesn't auto-pull)
  TEST_ASSERT_EQUAL_MESSAGE(0, stored_items.size(),
    "Pullable sink should not auto-pull");

  // Pull to trigger storage
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, stored_items.size(),
    "Pull should trigger storage");

  // Update state and pull again
  input_state.set(make_table_sink_input(std::string("second"), 100), false);
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, stored_items.size(),
    "Second pull should store new value");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("second", stored_items[1].first.c_str(),
    "Second key should be correct");
  TEST_ASSERT_EQUAL_MESSAGE(100, stored_items[1].second,
    "Second value should be correct");
}

void test_table_sink_with_quality_filter() {
  // Test that store function can reject values based on quality.
  struct Record {
    int data;
    float quality;  // Lower is better
  };

  std::map<std::string, Record> storage;

  auto quality_store = [&storage](const std::string& key, const Record& value) {
    auto it = storage.find(key);
    if (it == storage.end() || value.quality < it->second.quality) {
      storage[key] = value;
      return true;
    }
    return false;  // Rejected: existing is better
  };

  State<TableSinkInput<std::string, Record>> input_state(
    make_table_sink_input(std::string("key1"), Record{10, 0.5f})
  );

  auto sink = table_sink_pullable<std::string, Record>(quality_store);
  pull_fn pull = sink(input_state.get_source_fn(false));

  // First insert should succeed
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, storage.size(), "First insert should succeed");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.5f, storage["key1"].quality,
    "Quality should be 0.5");

  // Worse quality should be rejected
  input_state.set(make_table_sink_input(std::string("key1"), Record{20, 0.8f}), false);
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(10, storage["key1"].data,
    "Worse quality should not overwrite");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.5f, storage["key1"].quality,
    "Quality should still be 0.5");

  // Better quality should be accepted
  input_state.set(make_table_sink_input(std::string("key1"), Record{30, 0.3f}), false);
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(30, storage["key1"].data,
    "Better quality should overwrite");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.3f, storage["key1"].quality,
    "Quality should be 0.3");
}

void test_table_sink_with_knn_storage() {
  // Test integration with KNN-style storage.
  struct Point {
    float x, y;
  };
  struct PidWeights {
    float kp, ki, kd;
  };

  std::vector<std::pair<Point, PidWeights>> storage;

  auto knn_store = [&storage](const Point& key, const PidWeights& value) {
    storage.emplace_back(key, value);
    return true;
  };

  State<TableSinkInput<Point, PidWeights>> input_state(
    make_table_sink_input(Point{1.0f, 2.0f}, PidWeights{0.5f, 0.1f, 0.05f})
  );

  auto sink = table_sink_pullable<Point, PidWeights>(knn_store);
  pull_fn pull = sink(input_state.get_source_fn(false));

  pull();

  TEST_ASSERT_EQUAL_MESSAGE(1, storage.size(), "Storage should have one entry");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 1.0f, storage[0].first.x, "X should be 1.0");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 2.0f, storage[0].first.y, "Y should be 2.0");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.5f, storage[0].second.kp, "Kp should be 0.5");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.1f, storage[0].second.ki, "Ki should be 0.1");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01f, 0.05f, storage[0].second.kd, "Kd should be 0.05");
}

void test_make_table_sink_input() {
  // Test the convenience function.
  auto input = make_table_sink_input(std::string("test_key"), 123);

  TEST_ASSERT_EQUAL_STRING_MESSAGE("test_key", input.key.c_str(),
    "Key should be set correctly");
  TEST_ASSERT_EQUAL_MESSAGE(123, input.value,
    "Value should be set correctly");
}

void test_table_sink_multiple_keys() {
  // Test storing multiple different keys.
  std::map<std::string, int> storage;

  auto store = [&storage](const std::string& key, const int& value) {
    storage[key] = value;
    return true;
  };

  State<TableSinkInput<std::string, int>> input_state(
    make_table_sink_input(std::string("key1"), 10)
  );

  auto sink = table_sink_pullable<std::string, int>(store);
  pull_fn pull = sink(input_state.get_source_fn(false));

  pull();  // Store key1
  input_state.set(make_table_sink_input(std::string("key2"), 20), false);
  pull();  // Store key2
  input_state.set(make_table_sink_input(std::string("key3"), 30), false);
  pull();  // Store key3

  TEST_ASSERT_EQUAL_MESSAGE(3, storage.size(), "Should have 3 entries");
  TEST_ASSERT_EQUAL_MESSAGE(10, storage["key1"], "key1 should be 10");
  TEST_ASSERT_EQUAL_MESSAGE(20, storage["key2"], "key2 should be 20");
  TEST_ASSERT_EQUAL_MESSAGE(30, storage["key3"], "key3 should be 30");
}

void test_table_sink_store_fn_return_value() {
  // Test that store function return value is properly handled.
  int call_count = 0;
  bool should_accept = true;

  auto conditional_store = [&call_count, &should_accept](const std::string& key, const int& value) {
    call_count++;
    return should_accept;
  };

  State<TableSinkInput<std::string, int>> input_state(
    make_table_sink_input(std::string("key"), 42)
  );

  auto sink = table_sink_pullable<std::string, int>(conditional_store);
  pull_fn pull = sink(input_state.get_source_fn(false));

  // Accept first call
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, call_count, "Store should be called once");

  // Reject second call
  should_accept = false;
  input_state.set(make_table_sink_input(std::string("key"), 100), false);
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, call_count, "Store should be called again");
  // Note: Even though store returns false, it's still called
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_table_sink_calls_store_fn);
  RUN_TEST(test_table_sink_pullable_returns_pull_fn);
  RUN_TEST(test_table_sink_with_quality_filter);
  RUN_TEST(test_table_sink_with_knn_storage);
  RUN_TEST(test_make_table_sink_input);
  RUN_TEST(test_table_sink_multiple_keys);
  RUN_TEST(test_table_sink_store_fn_return_value);
  UNITY_END();
}
