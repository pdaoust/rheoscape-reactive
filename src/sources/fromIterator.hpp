#pragma once

#include <array>
#include <functional>
#include <iterator>
#include <core_types.hpp>
#include <Endable.hpp>

namespace rheo::sources {

  template <typename TIter>
  source_fn<Endable<typename TIter::value_type>> fromIterator(TIter iBegin, TIter iEnd) {
    return [iBegin, iEnd](push_fn<Endable<typename TIter::value_type>> push) {
      return [iBegin, iEnd, push, i = iBegin, isEnded = false]() mutable {
        if (isEnded) {
          push(Endable<typename TIter::value_type>());
          return;
        }
        if (i < iEnd) {
          push(Endable(*i));
          ++ i;
          if (i == iEnd) {
            isEnded = true;
          }
        }
      };
    };
  }

}