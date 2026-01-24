#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  // Named callable for scan's push handler (with initial value)
  template<typename TAcc, typename TIn, typename ScanFn>
  struct scan_push_handler_with_initial {
    ScanFn scanner;
    push_fn<TAcc> push;
    mutable TAcc acc;

    RHEO_CALLABLE void operator()(TIn value) const {
      acc = scanner(acc, value);
      push(acc);
    }
  };

  // Named callable for scan's source binder (with initial value)
  template<typename TAcc, typename TIn, typename ScanFn>
  struct scan_source_binder_with_initial {
    source_fn<TIn> source;
    ScanFn scanner;
    TAcc initial;

    RHEO_CALLABLE pull_fn operator()(push_fn<TAcc> push) const {
      return source(scan_push_handler_with_initial<TAcc, TIn, ScanFn>{scanner, push, initial});
    }
  };

  // Named callable for scan's push handler (no initial value)
  template<typename T, typename ScanFn>
  struct scan_push_handler_no_initial {
    ScanFn scanner;
    push_fn<T> push;
    mutable std::optional<T> acc = std::nullopt;

    RHEO_CALLABLE void operator()(T value) const {
      if (!acc.has_value()) {
        acc.emplace(value);
      } else {
        acc.emplace(scanner(acc.value(), value));
        push(acc.value());
      }
    }
  };

  // Named callable for scan's source binder (no initial value)
  template<typename T, typename ScanFn>
  struct scan_source_binder_no_initial {
    source_fn<T> source;
    ScanFn scanner;

    RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
      return source(scan_push_handler_no_initial<T, ScanFn>{scanner, push});
    }
  };

  template <typename TAcc, typename TIn, typename ScanFn>
    requires concepts::Scanner<ScanFn, TAcc, TIn>
  RHEO_CALLABLE source_fn<TAcc> scan(source_fn<TIn> source, TAcc initial, ScanFn&& scanner) {
    return scan_source_binder_with_initial<TAcc, TIn, std::decay_t<ScanFn>>{
      source,
      std::forward<ScanFn>(scanner),
      initial
    };
  }

  template <typename T, typename ScanFn>
    requires concepts::Scanner<ScanFn, T, T>
  RHEO_CALLABLE source_fn<T> scan(source_fn<T> source, ScanFn&& scanner) {
    return scan_source_binder_no_initial<T, std::decay_t<ScanFn>>{
      source,
      std::forward<ScanFn>(scanner)
    };
  }

  // Pipe factory with initial value
  template <typename TAcc, typename ScanFn>
  auto scan(TAcc initial, ScanFn&& scanner)
  -> pipe_fn<TAcc, arg_of<ScanFn, 1>> {
    using TIn = arg_of<ScanFn, 1>;
    return [scanner = std::forward<ScanFn>(scanner), initial](source_fn<TIn> source) -> source_fn<TAcc> {
      return scan(std::move(source), initial, std::move(scanner));
    };
  }

  // Pipe factory without initial value
  template <typename ScanFn>
  auto scan(ScanFn&& scanner)
  -> pipe_fn<return_of<ScanFn>, arg_of<ScanFn, 0>> {
    using T = arg_of<ScanFn, 0>;
    return [scanner = std::forward<ScanFn>(scanner)](source_fn<T> source) -> source_fn<T> {
      return scan(std::move(source), std::move(scanner));
    };
  }

}