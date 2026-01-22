#pragma once

#include <functional>
#include <memory>
#include <core_types.hpp>
#include <util.hpp>

namespace rheo::operators {

  // Sample the second stream every time the first stream receives a value.
  // A combiner function can optionally be passed; the default just returns the sampled value.
  // This source ends if either stream ends.

  template <typename TEvent, typename TSample, typename TOut, typename CombineFn>
  struct sample_and_map_sample_push_handler {
    CombineFn combiner;
    push_fn<TOut> push;
    std::shared_ptr<std::optional<TEvent>> last_event_value;

    RHEO_NOINLINE void operator()(TSample sample_value) const {
      if (last_event_value->has_value()) {
        // Extract the event value and clear BEFORE pushing.
        // This prevents re-entrant pushes (e.g., from State.set() in downstream)
        // from emitting again with the same event.
        TEvent event_value = std::move(last_event_value->value());
        last_event_value->reset();
        push(combiner(std::move(event_value), sample_value));
      }
    }
  };

  template <typename TEvent>
  struct sample_and_map_event_push_handler {
    std::shared_ptr<std::optional<TEvent>> last_event_value;
    pull_fn pull_sample;

    RHEO_NOINLINE void operator()(TEvent event_value) const {
      last_event_value->emplace(event_value);
      pull_sample();
    }
  };

  template <typename TEvent, typename TSample, typename TOut, typename CombineFn>
  struct sample_and_map_source_binder {
    source_fn<TEvent> event_source;
    source_fn<TSample> sample_source;
    CombineFn combiner;

    RHEO_NOINLINE pull_fn operator()(push_fn<TOut> push) const {
      auto last_event_value = std::make_shared<std::optional<TEvent>>(std::nullopt);

      pull_fn pull_sample = sample_source(sample_and_map_sample_push_handler<TEvent, TSample, TOut, CombineFn>{
        combiner,
        std::move(push),
        last_event_value
      });

      return event_source(sample_and_map_event_push_handler<TEvent>{
        last_event_value,
        std::move(pull_sample)
      });
    }
  };

  template <typename TEvent, typename TSample, typename CombineFn>
  auto sample_and_map(
    source_fn<TEvent> event_source,
    source_fn<TSample> sample_source,
    CombineFn combiner
  ) -> source_fn<return_of<CombineFn>> {
    using TOut = return_of<CombineFn>;
    return sample_and_map_source_binder<TEvent, TSample, TOut, CombineFn>{
      std::move(event_source),
      std::move(sample_source),
      std::move(combiner)
    };
  }

  template <typename TEvent, typename TSample, typename CombineFn>
  auto sample_and_map(
    source_fn<TSample> sample_source,
    CombineFn combiner
  ) -> pipe_fn<return_of<CombineFn>, TEvent> {
    return [sample_source, combiner](source_fn<TEvent> event_source) {
      return sample_and_map(event_source, sample_source, combiner);
    };
  }

  template <typename TOut, typename TSample, typename TEvent>
  pipe_fn<TOut, TSample> sample_and_map_every(
    source_fn<TEvent> event_source,
    combine2_fn<TOut, TSample, TEvent> combiner = [](TSample sample, TEvent event) { return sample; }
  ) {
    return [event_source, combiner](source_fn<TSample> sample_source) {
      return sample(event_source, sample_source, combiner);
    };
  }

  template <typename TEvent, typename TSample>
  source_fn<TSample> sample(
    source_fn<TEvent> event_source,
    source_fn<TSample> sample_source
  ) {
    return sample_and_map(
      event_source,
      sample_source,
      [](TEvent event, TSample sample) { return sample; }
    );
  }

  template <typename TEvent, typename TSample>
  pipe_fn<TSample, TEvent> sample(
    source_fn<TSample> sample_source
  ) {
    return [sample_source](source_fn<TEvent> event_source) {
      return sample(event_source, sample_source);
    };
  }

  template <typename TSample, typename TEvent>
  pipe_fn<TSample, TSample> sample_every(
    source_fn<TEvent> event_source
  ) {
    return [event_source](source_fn<TSample> sample_source) {
      return sample(event_source, sample_source);
    };
  }

}