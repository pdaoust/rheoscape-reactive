#ifndef RHEOSCAPE_SAMPLE
#define RHEOSCAPE_SAMPLE

#include <functional>
#include <core_types.hpp>

// Sample the second stream every time the first stream receives a value.
// A combiner function can optionally be passed; the default just returns the sampled value.

template <typename TEvent, typename TSample, typename TOut>
source_fn<TOut> sample_(
  source_fn<TEvent> eventSource,
  source_fn<TSample> sampleSource,
  combine2_fn<TEvent, TSample, TOut> combiner = [](TEvent event, TSample sample) { return sample; }
) {
  return [eventSource, sampleSource, combiner](push_fn<TOut> push) {
    std::optional<TEvent> lastEventValue;
    
    pull_fn pullSample = sampleSource([combiner, push, &lastEventValue](TSample sampleValue) {
      if (lastEventValue.has_value()) {
        push(combiner(lastEventValue, sampleValue));
        // Clear the event value out in case the sample source pushes something.
        // This prevents us from pushing a sample that _isn't_ sampled on an event.
        lastEventValue = std::nullopt;
      }
    });

    return eventSource([&lastEventValue, pullSample](TEvent eventValue) {
      lastEventValue = eventValue;
      pullSample();
    });
  };
}

template <typename TEvent, typename TSample, typename TOut>
pipe_fn<TEvent, TOut> sample(
  source_fn<TSample> sampleSource,
  combine2_fn<TEvent, TSample, TOut> combiner = [](TEvent event, TSample sample) { return sample; }
) {
  return [sampleSource, combiner](source_fn<TEvent> eventSource) {
    return sample_(eventSource, sampleSource, combiner);
  };
}

template <typename TSample, typename TEvent, typename TOut>
pipe_fn<TSample, TOut> sampleEvery(
  source_fn<TEvent> eventSource,
  combine2_fn<TSample, TEvent, TOut> combiner = [](TSample sample, TEvent event) { return sample; }
) {
  return [eventSource, combiner](source_fn<TSample> sampleSource) {
    return sample_(eventSource, sampleSource, combiner);
  };
}

#endif