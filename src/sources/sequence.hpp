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

  namespace detail {

    template <typename T, typename PushFn>
    struct sequence_pull_handler {
      T i_begin;
      T i_end;
      PushFn push;
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
      using value_type = Endable<T>;
      T i_begin;
      T i_end;
      T step;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {
        auto state = std::make_shared<sequence_state<T>>(sequence_state<T>{i_begin, step, false});
        return sequence_pull_handler<T, PushFn>{i_begin, i_end, std::move(push), state};
      }
    };

  } // namespace detail

  template <typename T>
  auto sequence(T i_begin, T i_end, T step = 1) {
    return detail::sequence_source_binder<T>{i_begin, i_end, step};
  }

  template <typename T>
  auto sequence_open(T i_begin, T step = 1) {
    return operators::unwrap_endable(sequence(i_begin, std::numeric_limits<T>::max(), step));
  }

}
