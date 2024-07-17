#pragma once

#include <array>
#include <functional>
#include <iterator>
#include <core_types.hpp>

namespace rheo {

  template <typename TIter>
  source_fn<typename TIter::value_type> fromIterator(TIter iBegin, TIter iEnd) {
    return [iBegin, iEnd](push_fn<typename TIter::value_type> push, end_fn end) {
      return [iBegin, iEnd, push, end, i = iBegin]() mutable {
        if (i < iEnd) {
          push(*i);
          ++ i;
          if (i == iEnd) {
            end();
          }
        } else {
          end();
        }
      };
    };
  }

}