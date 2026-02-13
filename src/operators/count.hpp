#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>

namespace rheo::operators {

  template <typename T>
  RHEO_CALLABLE source_fn<size_t> count(source_fn<T> source) {

    struct SourceBinder {
      source_fn<T> source;

      RHEO_CALLABLE pull_fn operator()(push_fn<size_t> push) const {

        struct PushHandler {
          push_fn<size_t> push;
          mutable size_t counter = 0;

          RHEO_CALLABLE void operator()(T value) const {
            counter++;
            push(counter);
          }
        };

        return source(PushHandler{push});
      }
    };

    return SourceBinder{source};
  }

  template <typename T>
  RHEO_CALLABLE source_fn<TaggedValue<T, size_t>> tag_count(source_fn<T> source) {

    struct SourceBinder {
      source_fn<T> source;

      RHEO_CALLABLE pull_fn operator()(push_fn<TaggedValue<T, size_t>> push) const {

        struct PushHandler {
          push_fn<TaggedValue<T, size_t>> push;
          mutable size_t counter = 0;

          RHEO_CALLABLE void operator()(T value) const {
            counter++;
            push(TaggedValue<T, size_t>{value, counter});
          }
        };

        return source(PushHandler{push});
      }
    };

    return SourceBinder{source};
  }

  namespace detail {
    struct CountPipeFactory {
      template <typename T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
        return count(std::move(source));
      }
    };

    struct TagCountPipeFactory {
      template <typename T>
      RHEO_CALLABLE auto operator()(source_fn<T> source) const {
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
