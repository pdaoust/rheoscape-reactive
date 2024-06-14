#ifndef RHEOSCAPE_ZIP
#define RHEOSCAPE_ZIP

#include <functional>
#include <core_types.hpp>


template <typename TIn, typename TOther, typename TOut>
pipe_fn<TIn, TOut> zipWith(source_fn<TOther> other, std::function<TOut(TIn, TOther)> mapper) {
  std::optional<TIn> thisLastValue;
  std::optional<TOther> otherLastValue;
  return [other, mapper, &thisLastValue, &otherLastValue](source_fn<T> source) {
    return [other, source, mapper, &thisLastValue, &otherLastValue](push_fn<TOut> push) {
      source([push](TIn value) { push(value); });
      other([push](TOther value) { push(value); });
      return [](){};
    };
  };
}

#endif