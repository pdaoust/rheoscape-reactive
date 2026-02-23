#pragma once

#include <functional>
#include <memory>
#include <tuple>
#include <types/core_types.hpp>
#include <util/misc.hpp>

namespace rheoscape::operators {

  // Sample the second stream every time the first stream receives a value.
  // Returns a tuple of (event_value, sampled_value).
  // To transform the tuple, use: sample(...) | map(mapper)
  // This source ends if either stream ends.

  namespace detail {
    template <typename EventSourceT, typename SampleSourceT>
    struct SampleSourceBinder {
      using TEvent = source_value_t<EventSourceT>;
      using TSample = source_value_t<SampleSourceT>;
      using value_type = std::tuple<TEvent, TSample>;

      EventSourceT event_source;
      SampleSourceT sample_source;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        auto last_event_value = std::make_shared<std::optional<TEvent>>(std::nullopt);

        struct SamplePushHandler {
          PushFn push;
          std::shared_ptr<std::optional<TEvent>> last_event_value;

          RHEOSCAPE_CALLABLE void operator()(TSample sample_value) const {
            if (last_event_value->has_value()) {
              // Extract the event value and clear BEFORE pushing.
              // This prevents re-entrant pushes (e.g., from MemoryState.set() in downstream)
              // from emitting again with the same event.
              TEvent event_value = std::move(last_event_value->value());
              last_event_value->reset();
              push(std::make_tuple(std::move(event_value), std::move(sample_value)));
            }
          }
        };

        struct EventPushHandler {
          std::shared_ptr<std::optional<TEvent>> last_event_value;
          pull_fn pull_sample;

          RHEOSCAPE_CALLABLE void operator()(TEvent event_value) const {
            last_event_value->emplace(event_value);
            pull_sample();
          }
        };

        pull_fn pull_sample = sample_source(SamplePushHandler{
          std::move(push),
          last_event_value
        });

        return event_source(EventPushHandler{
          last_event_value,
          std::move(pull_sample)
        });
      }
    };
  }

  // Source function factory: sample(event_source, sample_source)
  //
  // Samples sample_source every time event_source pushes a value.
  // Returns a tuple of (event_value, sampled_value).
  template <typename EventSourceT, typename SampleSourceT>
    requires concepts::Source<EventSourceT> && concepts::Source<SampleSourceT>
  auto sample(EventSourceT event_source, SampleSourceT sample_source) {
    return detail::SampleSourceBinder<EventSourceT, SampleSourceT>{
      std::move(event_source),
      std::move(sample_source)
    };
  }

  namespace detail {
    // Pipe factory: source1 | sample(sample_source)
    //
    // Samples sample_source every time the piped event source pushes a value.
    // Returns a tuple of (event_value, sampled_value).
    template <typename SampleSourceT>
    struct SamplePipeFactory {
      SampleSourceT sample_source;

      template <typename EventSourceT>
        requires concepts::Source<EventSourceT>
      RHEOSCAPE_CALLABLE auto operator()(EventSourceT event_source) const {
        return sample(std::move(event_source), SampleSourceT(sample_source));
      }
    };

    // Pipe factory: sample_source | sample_every(event_source)
    //
    // The inverse of sample(): samples the piped source every time event_source pushes.
    // Returns a tuple of (event_value, sampled_value).
    template <typename EventSourceT>
    struct SampleEveryPipeFactory {
      EventSourceT event_source;

      template <typename SampleSourceT>
        requires concepts::Source<SampleSourceT>
      RHEOSCAPE_CALLABLE auto operator()(SampleSourceT sample_source) const {
        return sample(EventSourceT(event_source), std::move(sample_source));
      }
    };
  }

  template <typename SampleSourceT>
    requires concepts::Source<SampleSourceT>
  auto sample(SampleSourceT sample_source) {
    return detail::SamplePipeFactory<SampleSourceT>{std::move(sample_source)};
  }

  template <typename EventSourceT>
    requires concepts::Source<EventSourceT>
  auto sample_every(EventSourceT event_source) {
    return detail::SampleEveryPipeFactory<EventSourceT>{std::move(event_source)};
  }

}
