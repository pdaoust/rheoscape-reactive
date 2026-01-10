#pragma once

#include <array>
#include <functional>
#include <iterator>
#include <memory>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::sources {

  template <typename TIter>
  struct from_iterator_state {
    TIter i;
    bool isEnded = false;
  };

  template <typename TIter>
  struct from_iterator_pull_handler {
    TIter iEnd;
    push_fn<Endable<typename TIter::value_type>> push;
    std::shared_ptr<from_iterator_state<TIter>> state;

    RHEO_NOINLINE void operator()() const {
      using T = typename TIter::value_type;
      if (state->isEnded) {
        push(Endable<T>());
        return;
      }
      if (state->i < iEnd) {
        push(Endable<T>(*state->i, state->i + 1 == iEnd));
        ++state->i;
        if (state->i == iEnd) {
          state->isEnded = true;
        }
      }
    }
  };

  template <typename TIter>
  struct from_iterator_source_binder {
    TIter iBegin;
    TIter iEnd;

    RHEO_NOINLINE pull_fn operator()(push_fn<Endable<typename TIter::value_type>> push) const {
      auto state = std::make_shared<from_iterator_state<TIter>>(from_iterator_state<TIter>{iBegin, false});
      return from_iterator_pull_handler<TIter>{iEnd, std::move(push), state};
    }
  };

  template <typename TIter>
  source_fn<Endable<typename TIter::value_type>> fromIterator(TIter iBegin, TIter iEnd) {
    return from_iterator_source_binder<TIter>{iBegin, iEnd};
  }

}