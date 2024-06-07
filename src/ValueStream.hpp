#ifndef VALUE_STREAM_HPP
#define VALUE_STREAM_HPP

#include <chrono>
#include <functional>
#include <optional>
#include <Bindable.hpp>
#include <TSValue.hpp>
#include <EventStream.hpp>

template <typename T>
class ValueStream;

template <typename T>
class ValueStream : public Bindable<ValueStream<T>> {
  private:
    std::function<T(mono_time_point)> _resolve;

  public:
    ValueStream(std::function<T(mono_time_point)> resolve)
    : _resolve(resolve)
    { }

    T read(mono_time_point ts) {
      return _resolve(ts);
    }

    ValueStream<MTSValue<T>> withTS() {
      return ValueStream<MTSValue<T>>([this](mono_time_point ts) {
        return MTSValue(ts, this->read(ts));
      });
    }

    template <typename TOut>
    ValueStream<TOut> map(std::function<TOut(T)> mapper) {
      return ValueStream<TOut>([this, mapper](mono_time_point ts) {
        return mapper(this->read(ts));
      });
    }

    template <typename TAcc>
    ValueStream<TAcc> fold(
      std::function<TAcc(TAcc, T)> folder,
      TAcc initialValue
    ) {
      TAcc lastValue = initialValue;
      return ValueStream<TAcc>([this, folder, &lastValue](mono_time_point ts) {
        lastValue = folder(lastValue, this->read(ts));
        return lastValue;
      });
    }

    ValueStream<T> reduce(std::function<T(T, T)> reducer) {
      std::optional<T> lastValue;
      return ValueStream<T>([this, reducer, &lastValue](mono_time_point ts) {
        T nextValue = this->read(ts);
        if (!lastValue.has_value()) {
          lastValue = nextValue;
        } else {
          lastValue = reducer(nextValue);
        }
        return lastValue.value();
      });
    }

    ValueStream<std::vector<T>> collect(std::initializer_list<ValueStream<T>*> others) {
      return ValueStream<T>([this, others](mono_time_point ts) {
        std::vector<T> collected;
        collected.push_back(this->read(ts));
        for (ValueStream<T>* other : others) {
          collected.push_back(other->read(ts));
        }
        return collected;
      });
    }

    template <size_t Len>
    ValueStream<std::array<T, Len + 1>> collect(std::array<ValueStream<T>*, Len> others) {
      return ValueStream<std::array<T, Len + 1>>([this, others](mono_time_point ts) {
        std::array<T, Len + 1> collected;
        collected[0] = this->read(ts);
        for (size_t i = 1; i <= Len; i ++) {
          ValueStream<T>* other = others[i - 1];
          collected[i] = other->read(ts);
        }
        return collected;
      });
    }

    template <typename TOther>
    ValueStream<std::tuple<T, TOther>> zip(ValueStream<TOther> other) {
      return ValueStream<std::tuple<T, TOther>>([this, other](mono_time_point ts) {
        return std::tuple<T, TOther>(this->read(ts), other.read(ts));
      });
    }

    template <typename TOther1, typename TOther2>
    ValueStream<std::tuple<T, TOther1, TOther2>> zip(ValueStream<TOther1> other1, ValueStream<TOther2> other2) {
      return ValueStream<std::tuple<T, TOther1, TOther2>>([this, other1, other2](mono_time_point ts) {
        return std::tuple<T, TOther1, TOther2>(this->read(ts), other1.read(ts), other2.read(ts));
      });
    }

    ValueStream<T> tap(std::function<void(ValueStream<T>*)> tapper) {
      tapper(this);
      return this;
    }

    ValueStream<std::optional<T>> filter(std::function<bool(T)> filterer) {
      return ValueStream<std::optional<T>>([this, filterer](mono_time_point ts) {
        T value = this->read(ts);
        return filterer(value)
          ? (std::optional<T>)value
          : std::nullopt;
      });
    }

    // FIXME: Memory leaks all around?????
    std::tuple<Runnable, EventStream<T>> watch() {
      T lastSeenValue;
      Runnable runnable;
      EventStream<T> eventStream([&runnable, &lastSeenValue](std::function<void(MTSValue<T>)> emit) {
        runnable = Runnable([&lastSeenValue, emit](mono_time_point ts) {
          T nextValue = read(ts);
          if (nextValue != lastSeenValue) {
            emit(MTSValue<T>(ts, nextValue));
            lastSeenValue = nextValue;
          }
        });
      });
      return std::tuple(runnable, eventStream);
    }
};

#endif