#pragma once

#include <functional>
#include <types/core_types.hpp>

namespace rheoscape::operators {

  namespace detail {
    template <typename SourceT>
    struct CountSourceBinder {
      using T = source_value_t<SourceT>;
      using value_type = size_t;

      SourceT source;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          PushFn push;
          mutable size_t counter = 0;

          RHEOSCAPE_CALLABLE void operator()(T) const {
            counter++;
            push(counter);
          }
        };

        return source(PushHandler{std::move(push)});
      }
    };

    template <typename SourceT>
    struct TagCountSourceBinder {
      using T = source_value_t<SourceT>;
      using value_type = std::tuple<T, size_t>;

      SourceT source;

      template <typename PushFn>
      RHEOSCAPE_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          PushFn push;
          mutable size_t counter = 0;

          RHEOSCAPE_CALLABLE void operator()(T value) const {
            counter++;
            push(std::tuple<T, size_t>{value, counter});
          }
        };

        return source(PushHandler{std::move(push)});
      }
    };
  }

  template <typename SourceT>
    requires concepts::Source<SourceT>
  RHEOSCAPE_CALLABLE auto count(SourceT source) {
    return detail::CountSourceBinder<SourceT>{std::move(source)};
  }

  template <typename SourceT>
    requires concepts::Source<SourceT>
  RHEOSCAPE_CALLABLE auto tag_count(SourceT source) {
    return detail::TagCountSourceBinder<SourceT>{std::move(source)};
  }

  namespace detail {
    struct CountPipeFactory {
      using is_pipe_factory = void;
      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return count(std::move(source));
      }
    };

    struct TagCountPipeFactory {
      using is_pipe_factory = void;
      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEOSCAPE_CALLABLE auto operator()(SourceT source) const {
        return tag_count(std::move(source));
      }
    };
  }

  inline auto count() {
    return detail::CountPipeFactory{};
  }

  inline auto tag_count() {
    return detail::TagCountPipeFactory{};
  }

}
