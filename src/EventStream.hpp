#ifndef EVENT_STREAM_HPP
#define EVENT_STREAM_HPP

#include <chrono>
#include <functional>
#include <optional>
#include <variant>
#include <Bindable.hpp>
#include <Runnable.hpp>
#include <TSValue.hpp>
#include <ValueStream.hpp>

template <typename T>
class ValueStream;

template <typename T>
class EventStream;

template <typename T>
class EventStream : public Bindable<EventStream<T>> {
  private:
    std::vector<std::function<void(MTSValue<T>)>> _subscribers;

    void _emit(MTSValue<T> event) {
      for (auto subscriber : _subscribers) {
        subscriber(event);
      }
    }

  public:
    EventStream(std::function<void(std::function<void(MTSValue<T>)>)> producer) {
      producer(_emit);
    }

    void addSubscriber(std::function<void(MTSValue<T>)> subscriber) {
      _subscribers.push_back(subscriber);
    }

    EventStream<MTSValue<T>> withTS() {
      return EventStream<MTSValue<T>>([](std::function<void(MTSValue<T>)> emit) {
        addSubscriber([emit](MTSValue<T> event) {
          emit(event);
        });
      });
    }

    template <typename TOut>
    EventStream<TOut> map(std::function<TOut(T)> mapper) {
      return EventStream<TOut>([mapper](std::function<void(MTSValue<T>)> emit) {
        addSubscriber([mapper, emit](MTSValue<T> event) {
          emit(MTSValue<T>(event.ts, mapper(event)));
        });
      });
    }

    template <typename TAcc>
    EventStream<TAcc> fold(std::function<TAcc(TAcc, T)> folder, TAcc initialValue) {
      return EventStream<TAcc>([folder, initialValue](std::function<void(MTSValue<T>)> emit) {
        TAcc lastValue = initialValue;
        addSubscriber([folder, &lastValue, emit](MTSValue<T> event) {
          lastValue = folder(lastValue, event);
          emit(MTSValue<TAcc>(event.ts, lastValue));
        });
      });
    }

    EventStream<T> reduce(std::function<T(MTSValue<T>, MTSValue<T>)> reducer) {
      return EventStream<T>([reducer](std::function<void(MTSValue<T>)> emit) {
        std::optional<T> lastValue;
        addSubscriber([reducer, &lastValue, emit](MTSValue<T> event) {
          if (!lastValue.has_value()) {
            lastValue = event;
          } else {
            lastValue = reducer(event);
          }
          emit(MTSValue<T>(event.ts, lastValue));
        });
      });
    }

    EventStream<T> filter(std::function<bool(T)> filterer) {
      return EventStream<T>([filterer](std::function<void(MTSValue<T>)> emit) {
        addSubscriber([filterer, emit](MTSValue<T> event) {
          if (filterer(event.value)) {
            emit(event);
          }
        });
      });
    }

    EventStream<T> collect(std::initializer_list<EventStream<T>*> others) {
      return EventStream<T>([others](std::function<void(MTSValue<T>)> emit) {
        std::function<void(MTSValue<T>)> subscriber = [emit](MTSValue<T> event) {
          emit(event);
        };
        addSubscriber(subscriber);
        for (EventStream<T>* other : others) {
          other->addSubscriber(subscriber);
        }
      });
    }

    template <typename TOther>
    EventStream<std::variant<T, TOther>> merge(EventStream<TOther>* other) {
      return EventStream<std::variant<T, TOther>>([other](std::function<void(MTSValue<std::variant<T, TOther>>)> emit) {
        addSubscriber([emit](MTSValue<T> event) {
          emit(MTSValue<std::variant<T, TOther>>(event.time, event.value));
        });
        other.addSubscriber([emit](MTSValue<TOther> event) {
          emit(MTSValue<std::variant<T, TOther>>(event.time, event.value));
        });
      });
    }

    template <typename TOther>
    EventStream<std::tuple<T, TOther>> zip(EventStream<TOther> other) {
      return EventStream<std::tuple<T, TOther>>([other](std::function<void(MTSValue<T>)> emit) {
        std::optional<MTSValue<T>> lastEventForThis;
        std::optional<MTSValue<TOther>> lastEventForOther;
        addSubscriber([&lastEventForThis, lastEventForOther, emit](MTSValue<T> event) {
          lastEventForThis = event;
          if (lastEventForOther.has_value()) {
            emit(MTSValue<std::tuple<T, TOther>>(event.time, std::tuple<T, TOther>(event.value, lastEventForOther.value().value)));
          }
        });
        other.addSubscriber([lastEventForThis, &lastEventForOther, emit](MTSValue<TOther> event) {
          lastEventForOther = event;
          if (lastEventForThis.has_value()) {
            emit(MTSValue<std::tuple<T, TOther>>(event.time, std::tuple<T, TOther>(lastEventForThis.value().value, event.value)));
          }
        });
      });
    }

    // TODO: implement operators as needed
    // * zip: emit event of tuple of last value of all zipped events any time a new event comes in
    // * mapTo: replace event's value with a constant
    // * take and skip
    // * startWith and endWhen
    // * tap: like addSubscriber but in a chained style

    ValueStream<std::optional<T>> remember() {
      std::optional<T> lastSeenValue;
      addSubscriber([&lastSeenValue](MTSValue<T> event) {
        lastSeenValue = event.value;
      });
      return ValueStream<std::optional<T>>([lastSeenValue](mono_time_point ts) {
        return lastSeenValue;
      });
    }
};

#endif