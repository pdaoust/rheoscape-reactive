#pragma once

#include <functional>
#include <memory>
#include <core_types.hpp>

namespace rheo {

  // Share one stream among many sinks.
  // This factory binds to the supplied source
  // (that is, it becomes the sink for the source)
  // so that its stream can be shared among many possible subscribers.
  // When the source pushes, all subscribers get the same value.
  // When any subscriber pulls, it's the only one that gets a value.

  template <typename T>
  void share(source_fn<T> source, std::initializer_list<sink_fn<T>> sinks) {
    auto sinkThatPulled = std::make_shared<std::optional<size_t>>();

    auto pull = std::make_shared(source(
      [sharedSinks, sinkThatPulled](T value) {
        if (sinkThatPulled.has_value()) {
          sharedSinks[sinkThatPulled.value()].push(value);
          sinkThatPulled = std::nullopt;
          return;
        }

        for (auto sink : sharedSinks) {
          sink.push(value);
        }
      },

      [sharedSinks, sinkThatPulled]() {
        for (auto sink : sharedSinks) {
          sink.end();
        }
      }
    ));

    for (size_t i = 0; i < sinks.size(); i ++) {
      sink([sinkThatPulled, pull, i](push_fn<T> push, end_fn end) {
        sinkThatPulled->emplace(i);
        pull();
      });
    }
  }

}