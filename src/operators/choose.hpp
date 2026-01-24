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
    std::shared_ptr<std::optional<TKey>> switch_state;
    TKey key;

    RHEO_CALLABLE void operator()(TVal value) const {
      // You'd think this is unnecessary, because this source fn's pull fn
      // guards against the wrong source being pulled.
      // Ah, but what about sources that do their own pushing?
      if (switch_state->has_value() && switch_state->value() == key) {
        push(value);
      }
    }
  };

  template <typename TKey>
  struct choose_switch_push_handler {
    std::shared_ptr<std::optional<TKey>> switch_state;

    RHEO_CALLABLE void operator()(TKey key) const {
      switch_state->emplace(key);
    }
  };

  template <typename TKey>
  struct choose_pull_handler {
    std::shared_ptr<std::optional<TKey>> switch_state;
    std::shared_ptr<std::map<TKey, pull_fn>> pull_value_fns;
    pull_fn pull_switch_source;

    RHEO_CALLABLE void operator()() const {
      // Pull this one first to give us a better chance of getting a desired switch state.
      pull_switch_source();
      // Only pull the value source that's switched on.
      if (switch_state->has_value() && pull_value_fns->count(switch_state->value())) {
        pull_value_fns->at(switch_state->value())();
      }
    }
  };

  template <typename TKey, typename TVal>
  struct choose_source_binder {
    std::map<TKey, source_fn<TVal>> value_source_map;
    source_fn<TKey> switch_source;

    RHEO_CALLABLE pull_fn operator()(push_fn<TVal> push) const {
      auto switch_state = std::make_shared<std::optional<TKey>>();
      auto pull_value_fns = std::make_shared<std::map<TKey, pull_fn>>();

      for (std::pair<TKey, source_fn<TVal>> const& pair : value_source_map) {
        pull_value_fns->insert_or_assign(
          pair.first,
          pair.second(choose_value_push_handler<TKey, TVal>{push, switch_state, pair.first})
        );
      }

      pull_fn pull_switch_source = switch_source(choose_switch_push_handler<TKey>{switch_state});

      return choose_pull_handler<TKey>{switch_state, pull_value_fns, std::move(pull_switch_source)};
    }
  };

  template <typename TKey, typename TVal>
  source_fn<TVal> choose(
    std::map<TKey, source_fn<TVal>> value_source_map,
    source_fn<TKey> switch_source
  ) {
    return choose_source_binder<TKey, TVal>{
      std::move(value_source_map),
      std::move(switch_source)
    };
  }

  // No pipe_fn factory for `choose` --
  // pipe_fn factories are used for chaining,
  // and you can't chain off a std::map.

  // A pipe that uses a switch source to switch the given value source.
  template <typename TKey, typename TVal>
  pipe_fn<TKey, TVal> choose_among(std::map<TKey, source_fn<TVal>> value_source_map) {
    return [value_source_map = std::move(value_source_map)](source_fn<TKey> switch_source) {
      return choose(std::move(value_source_map), std::move(switch_source));
    };
  }

}