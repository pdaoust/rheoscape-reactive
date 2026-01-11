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
  struct sampleAndMap_sample_push_handler {
    CombineFn combiner;
    push_fn<TOut> push;
    std::shared_ptr<std::optional<TEvent>> lastEventValue;

    RHEO_NOINLINE void operator()(TSample sampleValue) const {
      if (lastEventValue->has_value()) {
        push(combiner(lastEventValue->value(), sampleValue));
        // Clear the event value out in case the sample source pushes something.
        // This prevents us from pushing a sample that _isn't_ sampled on an event.
        lastEventValue->reset();
      }
    }
  };

  template <typename TEvent>
  struct sampleAndMap_event_push_handler {
    std::shared_ptr<std::optional<TEvent>> lastEventValue;
    pull_fn pullSample;

    RHEO_NOINLINE void operator()(TEvent eventValue) const {
      lastEventValue->emplace(eventValue);
      pullSample();
    }
  };

  template <typename TEvent, typename TSample, typename TOut, typename CombineFn>
  struct sampleAndMap_source_binder {
    source_fn<TEvent> eventSource;
    source_fn<TSample> sampleSource;
    CombineFn combiner;

    RHEO_NOINLINE pull_fn operator()(push_fn<TOut> push) const {
      auto lastEventValue = std::make_shared<std::optional<TEvent>>(std::nullopt);

      pull_fn pullSample = sampleSource(sampleAndMap_sample_push_handler<TEvent, TSample, TOut, CombineFn>{
        combiner,
        std::move(push),
        lastEventValue
      });

      return eventSource(sampleAndMap_event_push_handler<TEvent>{
        lastEventValue,
        std::move(pullSample)
      });
    }
  };

  template <typename TEvent, typename TSample, typename CombineFn>
  auto sampleAndMap(
    source_fn<TEvent> eventSource,
    source_fn<TSample> sampleSource,
    CombineFn combiner
  ) -> source_fn<return_of<CombineFn>> {
    using TOut = return_of<CombineFn>;
    return sampleAndMap_source_binder<TEvent, TSample, TOut, CombineFn>{
      std::move(eventSource),
      std::move(sampleSource),
      std::move(combiner)
    };
  }

  template <typename TEvent, typename TSample, typename CombineFn>
  auto sampleAndMap(
    source_fn<TSample> sampleSource,
    CombineFn combiner
  ) -> pipe_fn<return_of<CombineFn>, TEvent> {
    return [sampleSource, combiner](source_fn<TEvent> eventSource) {
      return sampleAndMap(eventSource, sampleSource, combiner);
    };
  }

  template <typename TOut, typename TSample, typename TEvent>
  pipe_fn<TOut, TSample> sampleAndMapEvery(
    source_fn<TEvent> eventSource,
    combine2_fn<TOut, TSample, TEvent> combiner = [](TSample sample, TEvent event) { return sample; }
  ) {
    return [eventSource, combiner](source_fn<TSample> sampleSource) {
      return sample(eventSource, sampleSource, combiner);
    };
  }

  template <typename TEvent, typename TSample>
  source_fn<TSample> sample(
    source_fn<TEvent> eventSource,
    source_fn<TSample> sampleSource
  ) {
    return sampleAndMap(
      eventSource,
      sampleSource,
      [](TEvent event, TSample sample) { return sample; }
    );
  }

  template <typename TEvent, typename TSample>
  pipe_fn<TSample, TEvent> sample(
    source_fn<TSample> sampleSource
  ) {
    return [sampleSource](source_fn<TEvent> eventSource) {
      return sample(eventSource, sampleSource);
    };
  }

  template <typename TSample, typename TEvent>
  pipe_fn<TSample, TSample> sampleEvery(
    source_fn<TEvent> eventSource
  ) {
    return [eventSource](source_fn<TSample> sampleSource) {
      return sample(eventSource, sampleSource);
    };
  }

}