#ifndef RHEOSCAPE_SWITCH
#define RHEOSCAPE_SWITCH

#include <functional>
#include <map>
#include <core_types.hpp>

// Switch among a map of value sources with a switch source.
// Everything is off until the switch source pushes the first value that matches a map key.
// After that, it'll only push values if the switch source matches a key in the map.
template <typename TKey, typename TVal>
source_fn<TVal> switch_(std::map<TKey, source_fn<TVal>> valueSourceMap, source_fn<TVal> switchSource) {
  return [valueSourceMap, switchSource](push_fn<TVal> push) {
    std::optional<TKey> switchState;

    std::map<TKey, pull_fn> pullValueFns;
    for (std::pair<TKey, source_fn<TVal>> const& pair : valueSourceMap) {
      pullValueFns[pair.first] = pair.second([push, switchState, pair](TVal value) {
        if (switchState.has_value() && switchState.value() == pair.first) {
          push(value);
        }
      });
    }

    pull_fn pullSwitchSource = switchSource([&switchState](TKey key) {
      switchState = key;
    });

    return [switchState, pullValueFns, pullSwitchSource]() {
      // Pull this one first to give us a better chance of getting a desired switch state.
      pullSwitchSource();
      // Only pull the value source that's switched on.
      if (pullValueFns.count(switchState)) {
        pullValueFns[switchState]();
      }
    };
  };
}

// A pipe that uses a switch source to switch the given value source.
template <typename TKey, typename TVal>
pipe_fn<TKey, TVal> applySwitch(std::map<TKey, source_fn<TVal>> valueSourceMap) {
  return [valueSourceMap](source_fn<bool> switchSource) {
    return switch_(valueSourceMap, switchSource);
  };
}

#endif