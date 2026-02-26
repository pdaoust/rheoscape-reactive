#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <types/core_types.hpp>

namespace rheoscape::operators {

  namespace detail {
    template <typename SourceT, typename TAcc, typename ScanFnT>
    struct ScanWithInitialSourceBinder {
      using value_type = TAcc;

      SourceT source;
      ScanFnT scanner;
      TAcc initial;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {
        using TIn = source_value_t<SourceT>;

        struct PushHandler {
          ScanFnT scanner;
          PushFn push;
          mutable TAcc acc;

          RHEOSCAPE_CALLABLE void operator()(TIn value) const {
            acc = invoke_scanner_maybe_apply(scanner, std::move(acc), std::move(value));
            push(acc);
          }
        };

        return source(PushHandler{scanner, std::move(push), initial});
      }
    };

    template <typename SourceT, typename ScanFnT>
    struct ScanSourceBinder {
      using T = source_value_t<SourceT>;
      using value_type = T;

      SourceT source;
      ScanFnT scanner;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          ScanFnT scanner;
          PushFn push;
          mutable std::optional<T> acc = std::nullopt;

          RHEOSCAPE_CALLABLE void operator()(T value) const {
            if (!acc.has_value()) {
              acc.emplace(value);
            } else {
              acc.emplace(invoke_scanner_maybe_apply(scanner, std::move(acc.value()), std::move(value)));
              push(acc.value());
            }
          }
        };

        return source(PushHandler{scanner, std::move(push)});
      }
    };
  }

  template <typename SourceT, typename TAcc, typename ScanFn>
    requires concepts::Source<SourceT> && concepts::Scanner<ScanFn, TAcc, source_value_t<SourceT>>
  RHEOSCAPE_CALLABLE auto scan(SourceT source, TAcc initial, ScanFn&& scanner) {
    return detail::ScanWithInitialSourceBinder<SourceT, TAcc, std::decay_t<ScanFn>>{
      std::move(source),
      std::forward<ScanFn>(scanner),
      initial
    };
  }

  template <typename SourceT, typename ScanFn>
    requires concepts::Source<SourceT> && concepts::Scanner<ScanFn, source_value_t<SourceT>, source_value_t<SourceT>>
  RHEOSCAPE_CALLABLE auto scan(SourceT source, ScanFn&& scanner) {
    return detail::ScanSourceBinder<SourceT, std::decay_t<ScanFn>>{
      std::move(source),
      std::forward<ScanFn>(scanner)
    };
  }

  namespace detail {
    template <typename TAcc, typename ScanFn>
    struct ScanWithInitialPipeFactory {
      using is_pipe_factory = void;
      ScanFn scanner;
      TAcc initial;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::Scanner<ScanFn, TAcc, source_value_t<SourceT>>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return scan(std::move(source), initial, ScanFn(scanner));
      }
    };

    template <typename ScanFn>
    struct ScanPipeFactory {
      using is_pipe_factory = void;
      ScanFn scanner;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::Scanner<ScanFn, source_value_t<SourceT>, source_value_t<SourceT>>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
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
