#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  // Forward declaration
  template <typename T>
  class EventSource;

  // Named callable for EventSource's source binder
  template <typename T>
  struct event_source_source_binder {
    EventSource<T>* event_source;

    RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
      return event_source->add_sink(std::move(push));
    }
  };

  template <typename T>
  class EventSource {
    private:
      std::vector<push_fn<T>> _sinks;

    public:
      EventSource() {}

      void push(T value) {
        for (auto& sink : _sinks) {
          sink(value);
        }
      }

      // This can be used as-is as a source function.
      pull_fn add_sink(push_fn<T> push) {
        _sinks.push_back(push);

        return []() {
          // No-op pull function
        };
      }

      source_fn<T> get_source_fn() {
        return event_source_source_binder<T>{this};
      }
  };

}