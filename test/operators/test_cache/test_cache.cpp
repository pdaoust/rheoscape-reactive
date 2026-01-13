#include <memory>
#include <unity.h>
#include <core_types.hpp>
#include <operators/cache.hpp>

using namespace rheo;
using namespace rheo::operators;

void test_cache_caches_after_pushed() {
  source_fn<int> pull_once_and_give_up = [](push_fn<int> push) {
    return [push, has_pulled_once = false]() mutable {
      if (!has_pulled_once) {
        push(11);
        has_pulled_once = true;
      }
    };
  };
  auto cacher = cache(pull_once_and_give_up);
  int pushed_value = 0;
  pull_fn pull = cacher([&pushed_value](int v) { pushed_value = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushed_value, "should get initial value; this is nothing special");
  // Reset the pushed value to make sure it really does push again even when the upstream source won't push anymore.
  pushed_value = 0;
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushed_value, "should get cached value");
}

void test_cache_doesnt_push_cached_value_when_theres_something_to_push() {
  source_fn<int> pull_alternately = [](push_fn<int> push) {
    return [push, count = 1, just_pushed = false]() mutable {
      if (!just_pushed) {
        push(count);
        just_pushed = true;
        count ++;
      } else {
        just_pushed = false;
      }
    };
  };
  auto cacher = cache(pull_alternately);
  int pushed_value = 0;
  pull_fn pull = cacher([&pushed_value](int v) { pushed_value = v; });
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value, "should get first pushed value");
  pushed_value = 0;
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushed_value, "should get cached first value");
  pushed_value = 0;
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_value, "should get second pushed value");
  pushed_value = 0;
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushed_value, "should get cached second value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_cache_caches_after_pushed);
  RUN_TEST(test_cache_doesnt_push_cached_value_when_theres_something_to_push);
  UNITY_END();
}
