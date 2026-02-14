#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>

namespace rheo::operators {

  namespace detail {
    template <typename SourceT>
    struct CountSourceBinder {
      using T = source_value_t<SourceT>;
      using value_type = size_t;

      SourceT source;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          PushFn push;
          mutable size_t counter = 0;

          RHEO_CALLABLE void operator()(T) const {
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
      using value_type = TaggedValue<T, size_t>;

      SourceT source;

      template <typename PushFn>
      RHEO_CALLABLE auto operator()(PushFn push) const {

        struct PushHandler {
          PushFn push;
          mutable size_t counter = 0;

          RHEO_CALLABLE void operator()(T value) const {
            counter++;
            push(TaggedValue<T, size_t>{value, counter});
          }
        };

        return source(PushHandler{std::move(push)});
      }
    };
  }

  template <typename SourceT>
    requires concepts::Source<SourceT>
  RHEO_CALLABLE auto count(SourceT source) {
    return detail::CountSourceBinder<SourceT>{std::move(source)};
  }

  template <typename SourceT>
    requires concepts::Source<SourceT>
  RHEO_CALLABLE auto tag_count(SourceT source) {
    return detail::TagCountSourceBinder<SourceT>{std::move(source)};
  }

  namespace detail {
    struct CountPipeFactory {
      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEO_CALLABLE auto operator()(SourceT source) const {
        return count(std::move(source));
      }
    };

    struct TagCountPipeFactory {
      template <typename SourceT>
        requires concepts::Source<SourceT>
      RHEO_CALLABLE auto operator()(SourceT source) const {
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
