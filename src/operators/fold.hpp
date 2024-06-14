#ifndef RHEOSCAPE_FOLD
#define RHEOSCAPE_FOLD

#include <functional>
#include <core_types.hpp>

template <typename TIn, typename TAcc>
pipe_fn<TIn, TAcc> fold(TAcc initialAcc, std::function<TAcc(TAcc, TIn)> folder) {
  TAcc acc = initialAcc;
  return [folder, &acc](source_fn<TIn> source) {
    return [folder, source, &acc](push_fn<TAcc> push) {
      return source([folder, push, &acc](TIn value) {
        acc = folder(acc, value);
        push(acc);
      });
    };
  };
}

#endif