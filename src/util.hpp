#ifndef RHEOSCAPE_UTIL
#define RHEOSCAPE_UTIL

#include <functional>

template <typename Func>
auto fnPtrToStdFn(Func ptr) {
  return std::function<std::remove_pointer_t<Func>>(ptr);
}

#endif