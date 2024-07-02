#pragma once

#include <functional>
#include <vector>
#include <core_types.hpp>
#include <Arduino.h>

namespace rheo {

  // A class to make it easier to work with Rheoscape sources and sinks.
  // You give it a source function, and then you can use method chaining
  // to make it really easy to read a long pipe.
  // You can also bind multiple sinks to this single source,
  // yet if one sink pulls, the other sinks won't accidentally get pushed a value too.
  // Finally, you can call the `drip()` method (at regular intervals?)
  // to pull from the sink and push the value to all sinks.
  template <typename T>
  class Rheo {
    private:
      // These are all the sinks' push functions.
      std::vector<push_fn<T>> _pushFns;
      // And these are all the sinks' end functions.
      std::vector<end_fn> _endFns;
      // Because Rheo represents a source function,
      // it must also be able to return the source function's pull function.
      pull_fn _pullFn;
      // When one subscriber pulled, we don't want the other subscribers to suddenly get the value.
      // Remember which one pulled so we can route the push correctly.
      std::optional<size_t> _pushFnThatPulled;

      void _emit(T value) {
        if (_pushFnThatPulled.has_value()) {
          _pushFns[_pushFnThatPulled.value()](value);
          _pushFnThatPulled = std::nullopt;
          return;
        }

        for (push_fn<T> pushFn : _pushFns) {
          pushFn(value);
        }
      }
      
      void _end() {
        for (end_fn : _endFns) {
          endFn();
        }
      }

    public:
      Rheo(source_fn<T> producer) {
        _pullFn = producer(
          [this](T value) { this->_emit(value); },
          [this]() { this->_end(); }
        );
      }

      // This method has the signature of a source_fn<T>.
      // It can also be used to add subscribers.
      pull_fn sourceFn(push_fn<T> pushFn, end_fn endFn) {
        _pushFns.push_back(pushFn);
        _endFns.push_back(endFn);

        // Each sink gets a pull function
        // that allows Rheo to remember who pulled
        // and push to the right sink.
        size_t pushFnIndex = _pushFns.size() - 1;
        return [this, pushFnIndex]() {
          _pushFnThatPulled = pushFnIndex;
          _pullFn();
        };
      }

      // A more semantically obvious alias of sourceFn.
      pull_fn addSink(push_fn<T> pushFn, end_fn endFn) {
        return sourceFn(pushFn, endFn);
      }

      // This is what Rheo is for -- making it easy to use method chaining.
      template <typename TReturn>
      TReturn _(sink_fn<T, TReturn> sink) {
        return sink([this](push_fn<T> pushFn) { return this->sourceFn(pushFn); });
      }

      // And this even more so -- pipe functions let you chain a bunch of operations together.
      template <typename TOut>
      Rheo<TOut> _(pipe_fn<T, TOut> pipe) {
        return Rheo<TOut>(pipe([this](push_fn<T> pushFn) { return this->sourceFn(pushFn); }));
      }

      // Call this repeatedly (or on an interval) to turn the producer into a push source.
      // This is especially useful for interval timers.
      void drip() {
        _pullFn();
      }
  };

}