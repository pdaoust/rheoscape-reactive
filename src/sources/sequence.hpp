#pragma once

#include <array>
#include <functional>
#include <iterator>
#include <core_types.hpp>

namespace rheo::sources {

  template <typename T>
  source_fn<Endable<T>> sequence(T iBegin, T iEnd, T step = 1) {
    return [iBegin, iEnd, step](push_fn<T> push) {
      return [iBegin, iEnd, step, push, i = iBegin, isEnded = false]() mutable {
        if (isEnded) {
          push(Endable<T>());
          return;
        }
        bool isBackwards = (iBegin > iEnd);
        if (isBackwards && step > 0) {
          step *= -1;
        }
        if ((!isBackwards && i <= iEnd)
            || (isBackwards && i >= iEnd)) {
          push(i);
          i += step;
          if ((!isBackwards && i > iEnd)
              || (isBackwards && i < iEnd)) {
            isEnded = true;
          }
        }
      };
    };
  }

}