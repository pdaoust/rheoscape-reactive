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
    bool is_ended = false;
  };

  template <typename T>
  struct sequence_pull_handler {
    T i_begin;
    T i_end;
    push_fn<Endable<T>> push;
    std::shared_ptr<sequence_state<T>> state;

    RHEO_CALLABLE void operator()() const {
      if (state->is_ended) {
        push(Endable<T>());
        return;
      }
      bool is_backwards = (i_begin > i_end);
      if (is_backwards && state->step > 0) {
        state->step *= -1;
      }
      if ((!is_backwards && state->i <= i_end)
          || (is_backwards && state->i >= i_end)) {
        push(Endable<T>(state->i, state->i == i_end));
        state->i += state->step;
        if ((!is_backwards && state->i > i_end)
            || (is_backwards && state->i < i_end)) {
          state->is_ended = true;
        }
      }
    }
  };

  template <typename T>
  struct sequence_source_binder {
    T i_begin;
    T i_end;
    T step;

    RHEO_CALLABLE pull_fn operator()(push_fn<Endable<T>> push) const {
      auto state = std::make_shared<sequence_state<T>>(sequence_state<T>{i_begin, step, false});
      return sequence_pull_handler<T>{i_begin, i_end, std::move(push), state};
    }
  };

  template <typename T>
  source_fn<Endable<T>> sequence(T i_begin, T i_end, T step = 1) {
    return sequence_source_binder<T>{i_begin, i_end, step};
  }

  template <typename T>
  source_fn<T> sequence_open(T i_begin, T step = 1) {
    return operators::unwrap_endable(sequence(i_begin, std::numeric_limits<T>::max(), step));
  }

}