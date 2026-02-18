#pragma once

#include <array>
#include <functional>
#include <iterator>
#include <memory>
#include <types/core_types.hpp>
#include <types/Endable.hpp>

namespace rheoscape::sources {

  template <typename TIter>
  struct from_iterator_state {
    TIter i;
    bool is_ended = false;
  };

  namespace detail {

    template <typename TIter, typename PushFn>
    struct from_iterator_pull_handler {
      TIter i_end;
      PushFn push;
      std::shared_ptr<from_iterator_state<TIter>> state;

      RHEOSCAPE_CALLABLE void operator()() const {
        using T = typename TIter::value_type;
        if (state->is_ended) {
          push(Endable<T>());
          return;
        }
        if (state->i < i_end) {
          push(Endable<T>(*state->i, state->i + 1 == i_end));
          ++state->i;
          if (state->i == i_end) {
            state->is_ended = true;
          }
        }
      }
    };

    template <typename TIter>
    struct from_iterator_source_binder {
      using value_type = Endable<typename TIter::value_type>;
      TIter i_begin;
      TIter i_end;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        auto state = std::make_shared<from_iterator_state<TIter>>(from_iterator_state<TIter>{i_begin, false});
        return from_iterator_pull_handler<TIter, PushFn>{i_end, std::move(push), state};
      }
    };

  } // namespace detail

  template <typename TIter>
  auto from_iterator(TIter i_begin, TIter i_end) {
    return detail::from_iterator_source_binder<TIter>{i_begin, i_end};
  }

}
