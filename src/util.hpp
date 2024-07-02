#pragma once

#include <functional>
#include <core_types.hpp>

namespace rheo {

  template <typename Func>
  auto fnPtrToStdFn(Func ptr) {
    return std::function<std::remove_pointer_t<Func>>(ptr);
  }

  // A class to help with the ending of a
  // source function that take multiple upstream source functions
  // and ends when any of them end.
  struct EndAny {
    const end_fn downstream_end_fn;
    bool ended;
    const end_fn upstream_end_fn;

    EndAny(end_fn downstream_end_fn)
    :
      downstream_end_fn(downstream_end_fn),
      ended(false),
      upstream_end_fn([this]() {
        this->ended = true;
        this->downstream_end_fn();
      })
    { }
  };

}