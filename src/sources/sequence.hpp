#pragma once

#include <array>
#include <functional>
#include <iterator>
#include <memory>
#include <core_types.hpp>
#include <Endable.hpp>
#include <operators/unwrap.hpp>

namespace rheo::sources {

  template <typename T>
  struct sequence_state {
    T i;
    T step;
    bool isEnded = false;
  };

  template <typename T>
  struct sequence_pull_handler {
    T iBegin;
    T iEnd;
    push_fn<Endable<T>> push;
    std::shared_ptr<sequence_state<T>> state;

    RHEO_NOINLINE void operator()() const {
      if (state->isEnded) {
        push(Endable<T>());
        return;
      }
      bool isBackwards = (iBegin > iEnd);
      if (isBackwards && state->step > 0) {
        state->step *= -1;
      }
      if ((!isBackwards && state->i <= iEnd)
          || (isBackwards && state->i >= iEnd)) {
        push(Endable<T>(state->i, state->i == iEnd));
        state->i += state->step;
        if ((!isBackwards && state->i > iEnd)
            || (isBackwards && state->i < iEnd)) {
          state->isEnded = true;
        }
      }
    }
  };

  template <typename T>
  struct sequence_source_binder {
    T iBegin;
    T iEnd;
    T step;

    RHEO_NOINLINE pull_fn operator()(push_fn<Endable<T>> push) const {
      auto state = std::make_shared<sequence_state<T>>(sequence_state<T>{iBegin, step, false});
      return sequence_pull_handler<T>{iBegin, iEnd, std::move(push), state};
    }
  };

  template <typename T>
  source_fn<Endable<T>> sequence(T iBegin, T iEnd, T step = 1) {
    return sequence_source_binder<T>{iBegin, iEnd, step};
  }

  template <typename T>
  source_fn<T> sequenceOpen(T iBegin, T step = 1) {
    return operators::unwrapEndable(sequence(iBegin, std::numeric_limits<T>::max(), step));
  }

}