#include <memory>
#include <unity.h>
#include <core_types.hpp>
#include <operators/cache.hpp>

void test_cache_caches_after_pushed() {
  rheo::source_fn<int> pullOnceAndGiveUp = [](rheo::push_fn<int> push, rheo::end_fn _) {
    return [push, hasPulledOnce = false]() mutable {
      if (!hasPulledOnce) {
        push(11);
        hasPulledOnce = true;
      }
    };
  };
  auto cacher = rheo::cache(pullOnceAndGiveUp);
  int pushedValue = 0;
  rheo::pull_fn pull = cacher(
    [&pushedValue](int v) { pushedValue = v; },
    [](){}
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedValue, "should get initial value; this is nothing special");
  // Reset the pushed value to make sure it really does push again even when the upstream source won't push anymore.
  pushedValue = 0;
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedValue, "should get cached value");
}

void test_cache_caches_after_ended() {
  rheo::source_fn<int> pullOnceAndEnd = [](rheo::push_fn<int> push, rheo::end_fn end) {
    return [push, end, hasPulledOnce = false]() mutable {
      if (!hasPulledOnce) {
        push(11);
        hasPulledOnce = true;
        end();
      } else {
        end();
      }
    };
  };
  auto cacher = rheo::cache(pullOnceAndEnd);
  int pushedValue = 0;
  rheo::pull_fn pull = cacher(
    [&pushedValue](int v) { pushedValue = v; },
    [](){}
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedValue, "should get initial value; this is nothing special");
  // Reset the pushed value to make sure it really does push again even when the upstream source won't push anymore.
  pushedValue = 0;
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(11, pushedValue, "should get cached value");
}

void test_cache_doesnt_push_cached_value_when_theres_something_to_push() {
  rheo::source_fn<int> pullAlternately = [](rheo::push_fn<int> push, rheo::end_fn _) {
    return [push, count = 1, justPushed = false]() mutable {
      if (!justPushed) {
        push(count);
        justPushed = true;
        count ++;
      } else {
        justPushed = false;
      }
    };
  };
  auto cacher = rheo::cache(pullAlternately);
  int pushedValue = 0;
  rheo::pull_fn pull = cacher(
    [&pushedValue](int v) { pushedValue = v; },
    [](){}
  );
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "should get first pushed value");
  pushedValue = 0;
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(1, pushedValue, "should get cached first value");
  pushedValue = 0;
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedValue, "should get second pushed value");
  pushedValue = 0;
  pull();
  TEST_ASSERT_EQUAL_MESSAGE(2, pushedValue, "should get cached second value");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_cache_caches_after_pushed);
  RUN_TEST(test_cache_caches_after_ended);
  RUN_TEST(test_cache_doesnt_push_cached_value_when_theres_something_to_push);
  UNITY_END();
}
