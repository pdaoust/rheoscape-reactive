#pragma once

#include <functional>
#include <core_types.hpp>
#include <util.hpp>

namespace rheo::operators {

  // Sample the second stream every time the first stream receives a value.
  // A combiner function can optionally be passed; the default just returns the sampled value.
  // This source ends if either stream ends.

  template <typename TOut, typename TEvent, typename TSample>
  source_fn<TOut> sampleAndMap(
    source_fn<TEvent> eventSource,
    source_fn<TSample> sampleSource,
    combine2_fn<TOut, TEvent, TSample> combiner
  ) {
    return [eventSource, sampleSource, combiner](push_fn<TOut> push) {
      auto lastEventValue = std::make_shared<std::optional<TEvent>>(std::nullopt);
      
      pull_fn pullSample = sampleSource([combiner, push, lastEventValue](TSample sampleValue) {
        if (lastEventValue->has_value()) {
          push(combiner(lastEventValue->value(), sampleValue));
          // Clear the event value out in case the sample source pushes something.
          // This prevents us from pushing a sample that _isn't_ sampled on an event.
          lastEventValue->reset();
        }
      });

      return eventSource([lastEventValue, pullSample](TEvent eventValue) {
        lastEventValue->emplace(eventValue);
        pullSample();
      });
    };
  }

  template <typename TOut, typename TEvent, typename TSample>
  pipe_fn<TOut, TEvent> sampleAndMap(
    source_fn<TSample> sampleSource,
    combine2_fn<TOut, TEvent, TSample> combiner = [](TEvent event, TSample sample) { return sample; }
  ) {
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
    return sampleAndMap<TSample, TEvent, TSample>(
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