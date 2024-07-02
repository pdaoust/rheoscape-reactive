#pragma once

#include <array>
#include <functional>
#include <iterator>
#include <core_types.hpp>

namespace rheo {

  template <typename TVal, template <typename...> typename TIter>
  source_fn<TVal> fromIterable(TIter<TVal> iterable) {
    return [iterable](push_fn<TVal> push, end_fn end) {
      auto i = iterable.begin();
      return [iterable, push, end, i]() {
        if (i < iterable.end()) {
          push(*i);
          i ++;
        } else {
          end();
        }
      };
    };
  }

}