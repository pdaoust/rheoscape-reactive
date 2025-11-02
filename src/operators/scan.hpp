#pragma once
#include <functional>
#include <core_types.hpp>

namespace rheo::operators {
  
  template <typename TAcc, typename TIn, typename ScanFn>
  source_fn<TAcc> scan(source_fn<TIn> source, TAcc initial, ScanFn&& scanner) {
    return [source, scanner = std::forward<ScanFn>(scanner), acc = initial](push_fn<TAcc> push) mutable {
      return source([scanner, &acc, push](TIn value) mutable {
        acc = scanner(acc, value);
        push(acc);
      });
    };
  }

  template <typename T, typename ScanFn>
  source_fn<T> scan(source_fn<T> source, ScanFn&& scanner) {
    return [source, scanner = std::forward<ScanFn>(scanner)](push_fn<T> push) mutable {
      return source([scanner, acc = std::optional<T>(), push](T value) mutable {
        if (!acc.has_value()) {
          acc.emplace(value);
        } else {
          acc.emplace(scanner(acc.value(), value));
          push(acc.value());
        }
      });
    };
  }

  template <typename TAcc, typename ScanFn>
  auto scan(TAcc initial, ScanFn&& scanner)
  -> pipe_fn<TAcc, transformer_2_in_in_2_type_t<ScanFn>> {
    using TIn = transformer_2_in_in_2_type_t<ScanFn>;
    return [scanner = std::forward<ScanFn>(scanner), initial](source_fn<TIn> source) {
      return scan(source, initial, scanner);
    };
  }

  template <typename ScanFn>
  auto scan(ScanFn&& scanner)
  -> pipe_fn<transformer_2_in_out_type_t<ScanFn>, transformer_2_in_in_1_type_t<ScanFn>> {
    using T = transformer_2_in_in_1_type_t<ScanFn>;
    return [scanner = std::forward<ScanFn>(scanner)](source_fn<T> source) {
      return scan(source, scanner);
    };
  }

}