#pragma once
#include <functional>
#include <core_types.hpp>

namespace rheo::operators {

  // Map a value into zero or more values, then output all those values into the transformed stream.
  // Note that pulling on this operator simply pulls upstream.
  // That means that each pull might result in zero, one, or multiple pushed values!
  namespace detail {
    template <typename SourceT, typename FlatMapFnT>
    struct FlatMapSourceBinder {
      using TIn = source_value_t<SourceT>;
      using value_type = flat_map_value_t<FlatMapFnT, TIn>;

      SourceT source;
      FlatMapFnT mapper;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          FlatMapFnT mapper;
          PushFn push;

          RHEO_CALLABLE void operator()(TIn value) const {
            auto values = invoke_maybe_apply(mapper, std::move(value));
            for (value_type& v : values) {
              push(v);
            }
          }
        };

        return source(PushHandler{mapper, std::move(push)});
      }
    };
  } // namespace detail

  template <typename SourceT, typename FlatMapFn>
    requires concepts::Source<SourceT> && concepts::FlatMapper<FlatMapFn, source_value_t<SourceT>>
  RHEO_CALLABLE auto flat_map(SourceT source, FlatMapFn&& mapper) {
    return detail::FlatMapSourceBinder<SourceT, std::decay_t<FlatMapFn>>{
      std::move(source),
      std::forward<FlatMapFn>(mapper)
    };
  }

  namespace detail {
    template <typename FlatMapFnT>
    struct FlatMapPipeFactory {
      FlatMapFnT mapper;

      template <typename SourceT>
        requires concepts::Source<SourceT> && concepts::FlatMapper<FlatMapFnT, source_value_t<SourceT>>
      RHEO_CALLABLE auto operator()(SourceT source) const {
        return flat_map(std::move(source), FlatMapFnT(mapper));
      }
    };
  } // namespace detail

  template <typename FlatMapFn>
  auto flat_map(FlatMapFn&& mapper) {
    return detail::FlatMapPipeFactory<std::decay_t<FlatMapFn>>{
      std::forward<FlatMapFn>(mapper)
    };
  }

}
