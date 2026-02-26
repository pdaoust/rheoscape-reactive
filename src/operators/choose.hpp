#pragma once

#include <functional>
#include <map>
#include <memory>
#include <types/core_types.hpp>

namespace rheoscape::operators {

  // Switch among a map of value sources with a switch source.
  // Everything is off until the switch source pushes the first value that matches a map key.
  // After that, it'll only push values if the switch source matches a key in the map.
  //
  // This source function kinda consumes the `end` signals of all the upstream sources;
  // that is, it won't signal to the sink that it's ended
  // until _all_ of its upstream sources are also ended.
  // However, it will end if the switch key source ends, regardless of the other upstream sources.

  namespace detail {
    template <typename TKey, typename TVal>
    struct ChooseSourceBinder {
      using value_type = TVal;

      std::map<TKey, source_fn<TVal>> value_source_map;
      source_fn<TKey> switch_source;

      RHEOSCAPE_CALLABLE pull_fn operator()(push_fn<TVal> push) const {
        auto switch_state = std::make_shared<std::optional<TKey>>();
        auto pull_value_fns = std::make_shared<std::map<TKey, pull_fn>>();

        struct ValuePushHandler {
          push_fn<TVal> push;
          std::shared_ptr<std::optional<TKey>> switch_state;
          TKey key;

          RHEOSCAPE_CALLABLE void operator()(TVal value) const {
            // You'd think this is unnecessary, because this source fn's pull fn
            // guards against the wrong source being pulled.
            // Ah, but what about sources that do their own pushing?
            if (switch_state->has_value() && switch_state->value() == key) {
              push(value);
            }
          }
        };

        struct SwitchPushHandler {
          std::shared_ptr<std::optional<TKey>> switch_state;

          RHEOSCAPE_CALLABLE void operator()(TKey key) const {
            switch_state->emplace(key);
          }
        };

        struct PullHandler {
          std::shared_ptr<std::optional<TKey>> switch_state;
          std::shared_ptr<std::map<TKey, pull_fn>> pull_value_fns;
          pull_fn pull_switch_source;

          RHEOSCAPE_CALLABLE void operator()() const {
            // Pull this one first to give us a better chance of getting a desired switch state.
            pull_switch_source();
            // Only pull the value source that's switched on.
            if (switch_state->has_value() && pull_value_fns->count(switch_state->value())) {
              pull_value_fns->at(switch_state->value())();
            }
          }
        };

        for (std::pair<TKey, source_fn<TVal>> const& pair : value_source_map) {
          pull_value_fns->insert_or_assign(
            pair.first,
            pair.second(ValuePushHandler{push, switch_state, pair.first})
          );
        }

        pull_fn pull_switch_source = switch_source(SwitchPushHandler{switch_state});

        return PullHandler{switch_state, pull_value_fns, std::move(pull_switch_source)};
      }
    };
  }

  template <typename TKey, typename TVal, typename SwitchSourceT>
    requires concepts::Source<SwitchSourceT> &&
             std::is_same_v<source_value_t<SwitchSourceT>, TKey>
  auto choose(
    std::map<TKey, source_fn<TVal>> value_source_map,
    SwitchSourceT switch_source
  ) {
    return detail::ChooseSourceBinder<TKey, TVal>{
      std::move(value_source_map),
      source_fn<TKey>(std::move(switch_source))
    };
  }

  // No pipe_fn factory for `choose` --
  // pipe_fn factories are used for chaining,
  // and you can't chain off a std::map.

  namespace detail {
    // A pipe that uses a switch source to switch the given value source.
    template <typename TKey, typename TVal>
    struct ChooseAmongPipeFactory {
      using is_pipe_factory = void;
      std::map<TKey, source_fn<TVal>> value_source_map;

      template <typename SwitchSourceT>
        requires concepts::Source<SwitchSourceT> &&
                 std::is_same_v<source_value_t<SwitchSourceT>, TKey>
      RHEOSCAPE_CALLABLE auto operator()(SwitchSourceT switch_source) const {
        return choose(std::map(value_source_map), std::move(switch_source));
      }
    };
  }

  template <typename TKey, typename TVal>
  auto choose_among(std::map<TKey, source_fn<TVal>> value_source_map) {
    return detail::ChooseAmongPipeFactory<TKey, TVal>{std::move(value_source_map)};
  }

}
