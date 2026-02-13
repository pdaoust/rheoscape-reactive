#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <core_types.hpp>

namespace rheo::operators {

  template <typename TAcc, typename TIn, typename ScanFn>
    requires concepts::Scanner<ScanFn, TAcc, TIn>
  RHEO_CALLABLE source_fn<TAcc> scan(source_fn<TIn> source, TAcc initial, ScanFn&& scanner) {
    using ScanFnDecayed = std::decay_t<ScanFn>;

    struct SourceBinder {
      source_fn<TIn> source;
      ScanFnDecayed scanner;
      TAcc initial;

      RHEO_CALLABLE pull_fn operator()(push_fn<TAcc> push) const {

        struct PushHandler {
          ScanFnDecayed scanner;
          push_fn<TAcc> push;
          mutable TAcc acc;

          RHEO_CALLABLE void operator()(TIn value) const {
            acc = scanner(acc, value);
            push(acc);
          }
        };

        return source(PushHandler{scanner, push, initial});
      }
    };

    return SourceBinder{
      source,
      std::forward<ScanFn>(scanner),
      initial
    };
  }

  template <typename T, typename ScanFn>
    requires concepts::Scanner<ScanFn, T, T>
  RHEO_CALLABLE source_fn<T> scan(source_fn<T> source, ScanFn&& scanner) {
    using ScanFnDecayed = std::decay_t<ScanFn>;

    struct SourceBinder {
      source_fn<T> source;
      ScanFnDecayed scanner;

      RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {

        struct PushHandler {
          ScanFnDecayed scanner;
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

        return source(PushHandler{scanner, push});
      }
    };

    return SourceBinder{
      source,
      std::forward<ScanFn>(scanner)
    };
  }

  namespace detail {
    template <typename TAcc, typename ScanFn>
    struct ScanWithInitialPipeFactory {
      ScanFn scanner;
      TAcc initial;

      template <typename TIn>
        requires concepts::Scanner<ScanFn, TAcc, TIn>
      RHEO_CALLABLE auto operator()(source_fn<TIn> source) const {
        return scan(std::move(source), initial, ScanFn(scanner));
      }
    };

    template <typename ScanFn>
    struct ScanPipeFactory {
      ScanFn scanner;

      template <typename T>
        requires concepts::Scanner<ScanFn, T, T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return scan(std::move(source), ScanFn(scanner));
      }
    };
  }

  // Pipe factory with initial value
  template <typename TAcc, typename ScanFn>
  auto scan(TAcc initial, ScanFn&& scanner) {
    return detail::ScanWithInitialPipeFactory<TAcc, std::decay_t<ScanFn>>{
      std::forward<ScanFn>(scanner),
      initial
    };
  }

  // Pipe factory without initial value
  template <typename ScanFn>
  auto scan(ScanFn&& scanner) {
    return detail::ScanPipeFactory<std::decay_t<ScanFn>>{
      std::forward<ScanFn>(scanner)
    };
  }

}
