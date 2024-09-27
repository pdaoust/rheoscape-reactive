#pragma once

#include <memory>

namespace rheo {

  // The only purpose of this thing is to make it possible to change the value in a shared pointer.
  // FIXME: If I someday find out that you can do that without this, awesome.
  template <typename T>
  struct Wrapper {
    T value;
  };

  template <typename T>
  std::shared_ptr<Wrapper<T>> make_wrapper_shared(T value) {
    return std::make_shared<Wrapper<T>>(Wrapper<T> { value });
  }

  template <typename T>
  std::shared_ptr<Wrapper<T>> make_wrapper_shared() {
    return std::make_shared<Wrapper<T>>(Wrapper<T>{});
  }

}