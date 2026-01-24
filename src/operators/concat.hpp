#pragma once

#include <functional>
#include <core_types.hpp>
#include <Endable.hpp>
#include <types/Wrapper.hpp>

namespace rheo::operators {

  // Concatenate two sources into one,
  // starting the second one after the first one has ended.

  // First overload: Endable<T> + T -> T
  template <typename T>
  struct concat_source1_push_handler {
    push_fn<T> push;
    std::shared_ptr<Wrapper<bool>> source1HasEnded;

    RHEO_CALLABLE void operator()(Endable<T> value) const {
      if (value.has_value()) {
        push(value.value);
      } else {
        source1HasEnded->value = true;
      }
    }
  };

  template <typename T>
  struct concat_source2_push_handler {
    push_fn<T> push;
    std::shared_ptr<Wrapper<bool>> source1HasEnded;

    RHEO_CALLABLE void operator()(T value) const {
      if (source1HasEnded->value) {
        push(value);
      }
    }
  };

  template <typename T>
  struct concat_pull_handler {
    std::shared_ptr<Wrapper<bool>> source1HasEnded;
    pull_fn pull_source1;
    pull_fn pull_source2;

    RHEO_CALLABLE void operator()() const {
      if (!source1HasEnded->value) {
        pull_source1();
      } else {
        pull_source2();
      }
    }
  };

  template <typename T>
  struct concat_source_binder {
    source_fn<Endable<T>> source1;
    source_fn<T> source2;

    RHEO_CALLABLE pull_fn operator()(push_fn<T> push) const {
      auto source1HasEnded = make_wrapper_shared<bool>(false);

      pull_fn pull_source1 = source1(concat_source1_push_handler<T>{push, source1HasEnded});
      pull_fn pull_source2 = source2(concat_source2_push_handler<T>{push, source1HasEnded});

      return concat_pull_handler<T>{
        source1HasEnded,
        std::move(pull_source1),
        std::move(pull_source2)
      };
    }
  };

  template <typename T>
  source_fn<T> concat(source_fn<Endable<T>> source1, source_fn<T> source2) {
    return concat_source_binder<T>{std::move(source1), std::move(source2)};
  }

  template <typename T>
  pipe_fn<T, T> concat(source_fn<T> source2) {
    return [source2](source_fn<T> source1) {
      return concat(source1, source2);
    };
  }

  // Second overload: Endable<T> + Endable<T> -> Endable<T>
  template <typename T>
  struct concat_endable_source1_push_handler {
    push_fn<Endable<T>> push;
    std::shared_ptr<Wrapper<bool>> source1HasEnded;

    RHEO_CALLABLE void operator()(Endable<T> value) const {
      if (value.has_value()) {
        push(value);
      } else {
        source1HasEnded->value = true;
      }
    }
  };

  template <typename T>
  struct concat_endable_source2_push_handler {
    push_fn<Endable<T>> push;
    std::shared_ptr<Wrapper<bool>> source1HasEnded;

    RHEO_CALLABLE void operator()(Endable<T> value) const {
      if (source1HasEnded->value) {
        push(value);
      }
    }
  };

  template <typename T>
  struct concat_endable_pull_handler {
    std::shared_ptr<Wrapper<bool>> source1HasEnded;
    pull_fn pull_source1;
    pull_fn pull_source2;

    RHEO_CALLABLE void operator()() const {
      if (!source1HasEnded->value) {
        pull_source1();
      }
      if (source1HasEnded->value) {
        // We can't use an else here;
        // it's possible that the above attempt to pull source 1
        // has resulted in the discovery that it's ended.
        pull_source2();
      }
    }
  };

  template <typename T>
  struct concat_endable_source_binder {
    source_fn<Endable<T>> source1;
    source_fn<Endable<T>> source2;

    RHEO_CALLABLE pull_fn operator()(push_fn<Endable<T>> push) const {
      auto source1HasEnded = make_wrapper_shared<bool>(false);

      pull_fn pull_source1 = source1(concat_endable_source1_push_handler<T>{push, source1HasEnded});
      pull_fn pull_source2 = source2(concat_endable_source2_push_handler<T>{push, source1HasEnded});

      return concat_endable_pull_handler<T>{
        source1HasEnded,
        std::move(pull_source1),
        std::move(pull_source2)
      };
    }
  };

  template <typename T>
  source_fn<Endable<T>> concat(source_fn<Endable<T>> source1, source_fn<Endable<T>> source2) {
    return concat_endable_source_binder<T>{std::move(source1), std::move(source2)};
  }

}