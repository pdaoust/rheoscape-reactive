#ifndef RHEOSCAPE_CORE_TYPES
#define RHEOSCAPE_CORE_TYPES

#include <functional>

template <typename T>
using push_fn = std::function<void(T)>;

using pull_fn = std::function<void()>;

template <typename T>
using source_fn = std::function<pull_fn(push_fn<T>)>;

template <typename T, typename TReturn>
using sink_fn = std::function<TReturn(source_fn<T>)>;

template <typename T>
using cap_fn = sink_fn<T, void>;

template <typename TIn, typename TOut>
using pipe_fn = sink_fn<TIn, source_fn<TOut>>;

#endif