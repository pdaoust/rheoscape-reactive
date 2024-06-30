#ifndef RHEOSCAPE_SWITCH
#define RHEOSCAPE_SWITCH

#include <functional>
#include <map>
#include <unordered_set>
#include <core_types.hpp>

// Switch among a map of value sources with a switch source.
// Everything is off until the switch source pushes the first value that matches a map key.
// After that, it'll only push values if the switch source matches a key in the map.
//
// This source function kinda consumes the `end` signals of all the upstream sources;
// that is, it won't signal to the sink that it's ended
// until _all_ of its upstream sources are also ended.
// However, it will end if the switch key source ends.
template <typename TKey, typename TVal>
source_fn<TVal> choose(std::map<TKey, source_fn<TVal>> valueSourceMap, source_fn<TVal> switchSource) {
  return [valueSourceMap, switchSource](push_fn<TVal> push, end_fn end) {
    auto switchState = std::make_shared<std::optional<TKey>>();

    auto pullValueFns = std::make_shared<std::map<TKey, pull_fn>>();
    auto endedSources = std::make_shared<std::unordered_set<TKey>>();
    for (std::pair<TKey, source_fn<TVal>> const& pair : valueSourceMap) {
      pullValueFns[pair.first] = pair.second(
        [push, switchState, pair](TVal value) {
          // You'd think this is unnecessary, because this source fn's pull fn
          // guards against the wrong source being pulled.
          // Ah, but what about sources that do their own pushing?
          if (switchState->has_value() && switchState->value() == pair.first) {
            push(value);
          }
        },
        [&valueSourceMap, end, endedSources, pair]() {
          endedSources->emplace(pair.first);
          // End this source if all the upstream sources are ended.
          if (endedSources->size() == valueSourceMap->size()) {
            end();
          }
        }
      );
    }

    pull_fn pullSwitchSource = switchSource(
      [switchState](TKey key) { switchState->emplace(key); },
      // End this source if the switch key source ends.
      end
    );

    return [switchState, pullValueFns, pullSwitchSource]() {
      // Pull this one first to give us a better chance of getting a desired switch state.
      pullSwitchSource();
      // Only pull the value source that's switched on.
      if (pullValueFns->count(switchState)) {
        pullValueFns[switchState]();
      }
    };
  };
}

// No pipe_fn factory for `choose` --
// pipe_fn factories are used for chaining,
// and you can't chain off a std::map.

// A pipe that uses a switch source to switch the given value source.
template <typename TKey, typename TVal>
pipe_fn<TKey, TVal> chooseAmong(std::map<TKey, source_fn<TVal>> valueSourceMap) {
  return [valueSourceMap](source_fn<bool> switchSource) {
    return choose(valueSourceMap, switchSource);
  };
}

#endif