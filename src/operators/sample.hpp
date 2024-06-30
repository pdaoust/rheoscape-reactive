#ifndef RHEOSCAPE_SAMPLE
#define RHEOSCAPE_SAMPLE

#include <functional>
#include <core_types.hpp>
#include <util.hpp>

// Sample the second stream every time the first stream receives a value.
// A combiner function can optionally be passed; the default just returns the sampled value.
// This source ends if either stream ends.

template <typename TOut, typename TEvent, typename TSample>
source_fn<TOut> sample(
  source_fn<TEvent> eventSource,
  source_fn<TSample> sampleSource,
  combine2_fn<TOut, TEvent, TSample> combiner = [](TEvent event, TSample sample) { return sample; }
) {
  return [eventSource, sampleSource, combiner](push_fn<TOut> push, end_fn end) {
    auto lastEventValue = std::make_shared<std::optional<TEvent>>;
    auto endAny = std::make_shared<EndAny>();
    
    pull_fn pullSample = sampleSource(
      [combiner, push, lastEventValue, endAny](TSample sampleValue) {
        if (endAny->ended) {
          return;
        }
        if (lastEventValue->has_value()) {
          push(combiner(lastEventValue->value(), sampleValue));
          // Clear the event value out in case the sample source pushes something.
          // This prevents us from pushing a sample that _isn't_ sampled on an event.
          lastEventValue->reset();
        }
      },
      endAny->upstream_end_fn
    );

    return eventSource(
      [lastEventValue, pullSample, endAny](TEvent eventValue) {
        if (endAny->ended) {
          return;
        }
        lastEventValue->emplace(eventValue);
        pullSample();
      },
      endAny->upstream_end_fn
    );
  };
}

template <typename TOut, typename TEvent, typename TSample>
pipe_fn<TOut, TEvent> sample(
  source_fn<TSample> sampleSource,
  combine2_fn<TOut, TEvent, TSample> combiner = [](TEvent event, TSample sample) { return sample; }
) {
  return [sampleSource, combiner](source_fn<TEvent> eventSource) {
    return sample(eventSource, sampleSource, combiner);
  };
}

template <typename TOut, typename TSample, typename TEvent>
pipe_fn<TOut, TSample> sampleEvery(
  source_fn<TEvent> eventSource,
  combine2_fn<TOut, TSample, TEvent> combiner = [](TSample sample, TEvent event) { return sample; }
) {
  return [eventSource, combiner](source_fn<TSample> sampleSource) {
    return sample(eventSource, sampleSource, combiner);
  };
}

#endif