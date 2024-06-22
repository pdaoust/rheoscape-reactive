#ifndef RHEOSCAPE_CORE_TYPES
#define RHEOSCAPE_CORE_TYPES

#include <chrono>
#include <functional>

// A function that a sink provides to a source
// to allow the source to push a value to the sink.
template <typename T>
using push_fn = std::function<void(T)>;

// A function that a source provides to a sink
// to allow the sink to request a new value from the source.
using pull_fn = std::function<void()>;

// A function that produces values and pushes them to a sink.
// It needs to receive the sink's push function,
// and returns a pull function to the sink.
// It's usually created inside a factory that has a structure like this:
//
//   (params) => (push_fn) => (pull_fn)
template <typename T>
using source_fn = std::function<pull_fn(push_fn<T>)>;

// A function that receives a source function,
// binds itself to the source
// (that is, passes it a push function that handles values pushed to it
// and optionally uses its pull function),
// and optionally returns a value.
// This value is mostly useful for method chaining.
template <typename T, typename TReturn>
using sink_fn = std::function<TReturn(source_fn<T>)>;

// A sink function that doesn't return a value.
template <typename T>
using cap_fn = sink_fn<T, void>;

// A sink function that is also (or rather, returns) a source function!
// The signature for its factory (which is called an operator) usually looks like this:
//
//   (params) => (source_fn) => (push_fn) => (pull_fn)
template <typename TIn, typename TOut>
using pipe_fn = sink_fn<TIn, source_fn<TOut>>;

// Some types for common functional operators.
template <typename TIn, typename TOut>
using map_fn = std::function<TOut(TIn)>;

template <typename T>
using reduce_fn = std::function<T(T, T)>;

template <typename TIn, typename TAcc>
using fold_fn = std::function<TAcc(TAcc, TIn)>;

template <typename T>
using filter_fn = std::function<bool(T)>;

template <typename T>
using exec_fn = std::function<void(T)>;

template <typename T1, typename T2, typename TCombined>
using combine2_fn = std::function<TCombined(T1, T2)>;

template <typename T1, typename T2, typename T3, typename TCombined>
using combine3_fn = std::function<TCombined(T1, T2, T3)>;

template <typename T1, typename T2, typename T3, typename T4, typename TCombined>
using combine4_fn = std::function<TCombined(T1, T2, T3, T4)>;

// Shorthands for different kinds of time:
// monotonic and wall time, which are synonyms for
// steady time and system time.
using mono_duration = std::chrono::steady_clock::duration;
using mono_time_point = std::chrono::steady_clock::time_point;
using wall_duration = std::chrono::system_clock::duration;
using wall_time_point = std::chrono::system_clock::time_point;

// Can be used for buttons/reeds/switches that drive a pin high or low,
// or for pins that turn relays/FETS/etc on or off.
enum class SwitchState {
  off,
  on,
};

// Can be used for valves or doors.
enum class GateState {
  open,
  closed,
};

// Same here. Basically the same as the state,
// except you can also command it to stop opening or closing.
enum class GateCommand {
  open,
  close,
  stop,
};

#endif