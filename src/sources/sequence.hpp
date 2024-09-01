#pragma once

#include <array>
#include <functional>
#include <iterator>
#include <core_types.hpp>

namespace rheo {

  template <typename T>
  source_fn<T> sequence(T iBegin, T iEnd, T step) {
    return [iBegin, iEnd, step](push_fn<T> push, end_fn end) {
      return [iBegin, iEnd, step, push, end, i = iBegin]() mutable {
        if (i <= iEnd) {
          push(i);
          i += step;
          if (i > iEnd) {
            end();
          }
        } else {
          end();
        }
      };
    };
  }

}