#pragma once

#include <functional>
#include <map>
#include <memory>
#include <core_types.hpp>

namespace rheo::operators {

  // Switch among a map of value sources with a switch source.
  // Everything is off until the switch source pushes the first value that matches a map key.
  // After that, it'll only push values if the switch source matches a key in the map.
  //
  // This source function kinda consumes the `end` signals of all the upstream sources;
  // that is, it won't signal to the sink that it's ended
  // until _all_ of its upstream sources are also ended.
  // However, it will end if the switch key source ends, regardless of the other upstream sources.

  template <typename TKey, typename TVal>
  struct choose_value_push_handler {
    push_fn<TVal> push;
    std::shared_ptr<std::optional<TKey>> switchState;
    TKey key;

    RHEO_NOINLINE void operator()(TVal value) const {
      // You'd think this is unnecessary, because this source fn's pull fn
      // guards against the wrong source being pulled.
      // Ah, but what about sources that do their own pushing?
      if (switchState->has_value() && switchState->value() == key) {
        push(value);
      }
    }
  };

  template <typename TKey>
  struct choose_switch_push_handler {
    std::shared_ptr<std::optional<TKey>> switchState;

    RHEO_NOINLINE void operator()(TKey key) const {
      switchState->emplace(key);
    }
  };

  template <typename TKey>
  struct choose_pull_handler {
    std::shared_ptr<std::optional<TKey>> switchState;
    std::shared_ptr<std::map<TKey, pull_fn>> pullValueFns;
    pull_fn pullSwitchSource;

    RHEO_NOINLINE void operator()() const {
      // Pull this one first to give us a better chance of getting a desired switch state.
      pullSwitchSource();
      // Only pull the value source that's switched on.
      if (switchState->has_value() && pullValueFns->count(switchState->value())) {
        pullValueFns->at(switchState->value())();
      }
    }
  };

  template <typename TKey, typename TVal>
  struct choose_source_binder {
    std::map<TKey, source_fn<TVal>> valueSourceMap;
    source_fn<TKey> switchSource;

    RHEO_NOINLINE pull_fn operator()(push_fn<TVal> push) const {
      auto switchState = std::make_shared<std::optional<TKey>>();
      auto pullValueFns = std::make_shared<std::map<TKey, pull_fn>>();

      for (std::pair<TKey, source_fn<TVal>> const& pair : valueSourceMap) {
        pullValueFns->insert_or_assign(
          pair.first,
          pair.second(choose_value_push_handler<TKey, TVal>{push, switchState, pair.first})
        );
      }

      pull_fn pullSwitchSource = switchSource(choose_switch_push_handler<TKey>{switchState});

      return choose_pull_handler<TKey>{switchState, pullValueFns, std::move(pullSwitchSource)};
    }
  };

  template <typename TKey, typename TVal>
  source_fn<TVal> choose(
    std::map<TKey, source_fn<TVal>> valueSourceMap,
    source_fn<TKey> switchSource
  ) {
    return choose_source_binder<TKey, TVal>{
      std::move(valueSourceMap),
      std::move(switchSource)
    };
  }

  // No pipe_fn factory for `choose` --
  // pipe_fn factories are used for chaining,
  // and you can't chain off a std::map.

  // A pipe that uses a switch source to switch the given value source.
  template <typename TKey, typename TVal>
  pipe_fn<TKey, TVal> chooseAmong(std::map<TKey, source_fn<TVal>> valueSourceMap) {
    return [valueSourceMap = std::move(valueSourceMap)](source_fn<TKey> switchSource) {
      return choose(std::move(valueSourceMap), std::move(switchSource));
    };
  }

}