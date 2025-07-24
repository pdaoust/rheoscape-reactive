#pragma once

#include <array>
#include <functional>
#include <core_types.hpp>

namespace rheo::sources {

  template <typename SubscribeFn>
  auto fromObservable(SubscribeFn&& subscribeFn)
  -> source_fn<decltype(subscribeFn(std::declval<source_value_type_t<std::decay_t<SubscribeFn>>>()))> {
    using T = source_value_type_t<std::decay_t<SubscribeFn>>;
    return [subscribeFn = std::move(subscribeFn)](push_fn<T> push) {
      subscribeFn([push](T value) { push(value); });
      // It doesn't mean anything to pull from an observable.
      return [](){};
    };
  }

}