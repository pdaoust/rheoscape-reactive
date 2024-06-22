#ifndef RHEOSCAPE_FOLD
#define RHEOSCAPE_FOLD

#include <functional>
#include <core_types.hpp>

template <typename TIn, typename TAcc>
source_fn<TAcc> fold_(source_fn<TIn> source, TAcc initialAcc, fold_fn<TIn, TAcc> folder) {
  return [source, initialAcc, folder](push_fn<TAcc> push) {
    TAcc acc = initialAcc;
    return source([folder, &acc, push](TIn value) {
      acc = folder(acc, value);
      push(acc);
    });
  };
}

template <typename TIn, typename TAcc>
pipe_fn<TIn, TAcc> fold(TAcc initialAcc, fold_fn<TIn, TAcc> folder) {
  return [initialAcc, folder](source_fn<TIn> source) {
    return fold_(source, initialAcc, folder);
  };
}

#endif