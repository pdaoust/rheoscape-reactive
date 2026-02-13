#pragma once

#include <functional>
#include <memory>
#include <tuple>
#include <core_types.hpp>
#include <util.hpp>

namespace rheo::operators {

  // Sample the second stream every time the first stream receives a value.
  // Returns a tuple of (event_value, sampled_value).
  // To transform the tuple, use: sample(...) | map_tuple(mapper)
  // This source ends if either stream ends.

  // Source function factory: sample(event_source, sample_source)
  //
  // Samples sample_source every time event_source pushes a value.
  // Returns a tuple of (event_value, sampled_value).
  template <typename TEvent, typename TSample>
  source_fn<std::tuple<TEvent, TSample>> sample(
    source_fn<TEvent> event_source,
    source_fn<TSample> sample_source
  ) {
    using TOut = std::tuple<TEvent, TSample>;

    struct SourceBinder {
      source_fn<TEvent> event_source;
      source_fn<TSample> sample_source;

      RHEO_CALLABLE pull_fn operator()(push_fn<TOut> push) const {
        auto last_event_value = std::make_shared<std::optional<TEvent>>(std::nullopt);

        struct SamplePushHandler {
          push_fn<TOut> push;
          std::shared_ptr<std::optional<TEvent>> last_event_value;

          RHEO_CALLABLE void operator()(TSample sample_value) const {
            if (last_event_value->has_value()) {
              // Extract the event value and clear BEFORE pushing.
              // This prevents re-entrant pushes (e.g., from State.set() in downstream)
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

          RHEO_CALLABLE void operator()(TEvent event_value) const {
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

    return SourceBinder{
      std::move(event_source),
      std::move(sample_source)
    };
  }

  namespace detail {
    // Pipe factory: source1 | sample(sample_source)
    //
    // Samples sample_source every time the piped event source pushes a value.
    // Returns a tuple of (event_value, sampled_value).
    template <typename TSample>
    struct SamplePipeFactory {
      source_fn<TSample> sample_source;

      template <typename TEvent>
      RHEO_CALLABLE auto operator()(source_fn<TEvent> event_source) const {
        return sample(std::move(event_source), sample_source);
      }
    };

    // Pipe factory: sample_source | sample_every(event_source)
    //
    // The inverse of sample(): samples the piped source every time event_source pushes.
    // Returns a tuple of (event_value, sampled_value).
    template <typename TEvent>
    struct SampleEveryPipeFactory {
      source_fn<TEvent> event_source;

      template <typename TSample>
      RHEO_CALLABLE auto operator()(source_fn<TSample> sample_source) const {
        return sample(event_source, std::move(sample_source));
      }
    };
  }

  template <typename TSample>
  auto sample(source_fn<TSample> sample_source) {
    return detail::SamplePipeFactory<TSample>{sample_source};
  }

  template <typename TEvent>
  auto sample_every(source_fn<TEvent> event_source) {
    return detail::SampleEveryPipeFactory<TEvent>{event_source};
  }

}
