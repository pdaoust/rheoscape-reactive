#ifndef RHEOSCAPE_RHEO
#define RHEOSCAPE_RHEO

#include <functional>
#include <vector>
#include <core_types.hpp>
#include <Arduino.h>

template <typename T>
class Rheo {
  private:
    std::vector<push_fn<T>> _pushFns;
    pull_fn _pullFn;
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

  public:
    Rheo(source_fn<T> producer) {
      _pullFn = producer([this](T value) { this->_emit(value); });
    }

    pull_fn sourceFn(push_fn<T> pushFn) {
      _pushFns.push_back(pushFn);
      size_t pushFnIndex = _pushFns.size() - 1;
      return [this, pushFnIndex]() {
        _pushFnThatPulled = pushFnIndex;
        _pullFn();
      };
    }

    template <typename TReturn>
    TReturn _(sink_fn<T, TReturn> sink) {
      return sink([this](push_fn<T> pushFn) { return this->sourceFn(pushFn); });
    }

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

#endif