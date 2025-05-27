#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  template <typename TAcc, typename TIn>
  source_fn<TAcc> fold(source_fn<TIn> source, TAcc initialAcc, fold_fn<TAcc, TIn> folder) {
    return [source, initialAcc, folder](push_fn<TAcc> push, end_fn end) {
      return source(
        [folder, acc = initialAcc, push](TIn value) mutable {
          acc = folder(acc, value);
          push(acc);
        },
        end
      );
    };
  }

  template <typename TAcc, typename TIn>
  pipe_fn<TAcc, TIn> fold(TAcc initialAcc, fold_fn<TAcc, TIn> folder) {
    return [initialAcc, folder](source_fn<TIn> source) {
      return fold(source, initialAcc, folder);
    };
  }

}