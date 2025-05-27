#pragma once

#include <array>
#include <functional>
#include <iterator>
#include <core_types.hpp>

namespace rheo::sources {

  template <typename T>
  source_fn<T> sequence(T iBegin, std::optional<T> iEnd = std::nullopt, T step = 1) {
    return [iBegin, iEnd, step](push_fn<T> push, end_fn end) {
      return [iBegin, iEnd, step, push, end, i = iBegin]() mutable {
        if (iEnd.has_value() && iBegin == iEnd.value()) {
          end();
          return;
        }
        bool isBackwards = (iEnd.has_value() && iBegin > iEnd.value());
        if (isBackwards && step > 0) {
          step *= -1;
        }
        if ((!isBackwards && (!iEnd.has_value() || i <= iEnd.value()))
            || (isBackwards && (!iEnd.has_value() || i >= iEnd.value()))) {
          push(i);
          i += step;
          if ((!isBackwards && iEnd.has_value() && i > iEnd.value())
              || (isBackwards && iEnd.has_value() && i < iEnd.value())) {
            end();
          }
        } else {
          end();
        }
      };
    };
  }

}