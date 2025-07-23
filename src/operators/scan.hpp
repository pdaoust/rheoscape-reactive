#include <functional>
#include <core_types.hpp>

namespace rheo::operators {
  
  template <typename TAcc, typename TIn>
  source_fn<TAcc> scan(source_fn<TIn> source, fold_fn<TAcc, TIn> scanner, TAcc initial) {
    return [source, scanner, acc = initial](push_fn<TAcc> push) {
      return source([scanner, &acc, push](TIn value) mutable {
        acc = scanner(acc, value);
        push(acc);
      });
    };
  }

  template <typename T>
  source_fn<T> scan(source_fn<T> source, fold_fn<T, T> scanner) {
    return [source, scanner, acc = std::optional<T>()](push_fn<T> push) {
      return source([scanner, &acc, push](T value) mutable {
        if (!acc.has_value()) {
          acc.emplace(value);
        } else {
          acc.emplace(scanner(acc.value(), value));
          push(acc.value());
        }
      });
    };
  }

  template <typename TAcc, typename TIn>
  pipe_fn<TAcc, TIn> scan(fold_fn<TAcc, TIn> scanner, TAcc initial) {
    return [scanner, initial](source_fn<TIn> source) {
      return scan(source, scanner, initial);
    };
  }

  template <typename T>
  pipe_fn<T, T> scan(fold_fn<T, T> scanner) {
    return [scanner](source_fn<T> source) {
      return scan(source, scanner);
    };
  }

}