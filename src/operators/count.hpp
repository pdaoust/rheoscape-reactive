#pragma once

#include <functional>
#include <core_types.hpp>
#include <types/TaggedValue.hpp>

namespace rheo::operators {

  // Named callable for count's push handler
  template<typename T>
  struct count_push_handler {
    push_fn<size_t> push;
    mutable size_t counter = 0;

    RHEO_CALLABLE void operator()(T value) const {
      counter++;
      push(counter);
    }
  };

  // Named callable for count's source binder
  template<typename T>
  struct count_source_binder {
    source_fn<T> source;

    RHEO_CALLABLE pull_fn operator()(push_fn<size_t> push) const {
      return source(count_push_handler<T>{push});
    }
  };

  // Named callable for tag_count's push handler
  template<typename T>
  struct tag_count_push_handler {
    push_fn<TaggedValue<T, size_t>> push;
    mutable size_t counter = 0;

    RHEO_CALLABLE void operator()(T value) const {
      counter++;
      push(TaggedValue<T, size_t>{value, counter});
    }
  };

  // Named callable for tag_count's source binder
  template<typename T>
  struct tag_count_source_binder {
    source_fn<T> source;

    RHEO_CALLABLE pull_fn operator()(push_fn<TaggedValue<T, size_t>> push) const {
      return source(tag_count_push_handler<T>{push});
    }
  };

  template <typename T>
  RHEO_CALLABLE source_fn<size_t> count(source_fn<T> source) {
    return count_source_binder<T>{source};
  }

  template <typename T>
  RHEO_CALLABLE source_fn<TaggedValue<T, size_t>> tag_count(source_fn<T> source) {
    return tag_count_source_binder<T>{source};
  }

  // Pipe factory for count
  template<typename T>
  struct count_pipe_factory {
    RHEO_CALLABLE source_fn<size_t> operator()(source_fn<T> source) const {
      return count(std::move(source));
    }
  };

  template <typename T>
  pipe_fn<size_t, T> count() {
    return count_pipe_factory<T>{};
  }

  // Pipe factory for tag_count
  template<typename T>
  struct tag_count_pipe_factory {
    RHEO_CALLABLE source_fn<TaggedValue<T, size_t>> operator()(source_fn<T> source) const {
      return tag_count(std::move(source));
    }
  };

  template <typename T>
  pipe_fn<TaggedValue<T, size_t>, T> tag_count() {
    return tag_count_pipe_factory<T>{};
  }

}