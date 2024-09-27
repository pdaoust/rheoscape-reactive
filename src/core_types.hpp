#pragma once

#include <chrono>
#include <functional>

namespace rheo {

  // Rheoscape is not a library.
  // It's just a specification for hybrid reactive/pullable iterables
  // and how they should work.
  // Every source is a function that takes a `push` callback and an `end` callback,
  // and returns a `pull` callback.
  // A consumer, called a sink, figures out what it wants to do when a source pushes to it,
  // figures out what it wants to do when a source ends,
  // and then calls a source function with callbacks that embody that logic.
  // It receives a function that it can then use to pull new data if it likes.
  //
  // The source function can be called many times, each with a different sink,
  // but every act of binding should be considered a separate stream.
  // Therefore, you could think of a source function as a 'stream factory'.
  //
  // There are other conventions, which are built on the above source function and sink idea:
  //
  // * A **sink function** is a sink-as-a-function, naturally --
  //   it takes a source function and binds to it.
  // * A **pipe function** is a sink function
  //   that's also a source function factory --
  //   it binds to the sink, then returns a source function
  //   that transforms the upstream source in some way.
  //
  // SPECS
  //
  // * A source function SHOULD allow multiple sinks to bind to it,
  //   and SHOULD independent streams for each act of binding.
  //   (This is actually easier to do than creating one stream.)
  // * A source function MAY (and possibly SHOULD) end eagerly;
  //   that is, if it knows that a subsequent pull would only trigger an end
  //   (e.g., it's just pushed the last value in an array)
  //   it can call its downstream end function before the subsequent pull.
  // * If a source doesn't end, its source function signature still MUST accept an end function
  //   (but it doesn't ever have to call it).
  // * If a source can't be pulled, its source function signature still MUST return a pull function
  //   (but it doesn't have to do anything if it's called).
  // * A source function MUST call the end callback again
  //   if its pull function is called after it's already ended.
  // * A sink SHOULD stop trying to pull
  //   if a source function has already called the sink's end function.
  // * An end function MAY be idempotent.
  // * A pipe function MAY pass its downstream end callback to its upstream source functions
  //   and pass its upstream pull callbacks to its downstream sink function
  //   in whatever way makes sense -- that is, it doesn't need to pass them directly,
  //   but a pull and an end should do something that the consumer expects them to do.
  // * A source that can push and models a persistent state MAY push its last value at bind time
  //   but MUST NOT expect its sink to be ready to receive values at bind time;
  //   the sink may have to do some setup first.
  // * A source that models a stream of events SHOULD NOT push the last the last event
  //   to its sink at bind time.

  // A function that a sink provides to a source
  // to allow the source to push a value to the sink.
  template <typename T>
  using push_fn = std::function<void(T)>;

  // A function that takes nothing and returns nothing,
  // but when called, serves as a signal to a source or sink
  // that something should happen.
  using signal_fn = std::function<void()>;

  // A function that a source provides to a sink
  // to allow the sink to request a new value from the source.
  using pull_fn = signal_fn;

  // A function that a sink provides to a source
  // to allow the source to signal that there won't be any more values.
  using end_fn = signal_fn;

  // A function that produces values and pushes them to a sink.
  // It needs to receive a push function and an end function from the sink,
  // and returns a pull function to the sink.
  // It's usually created inside a factory that has a structure like this:
  //
  //   (params) => (push_fn) => (pull_fn)
  template <typename T>
  using source_fn = std::function<pull_fn(push_fn<T>, end_fn)>;

  // A function that receives a source function,
  // binds itself to the source
  // (that is, passes it a push function that handles values pushed to it
  // and optionally uses its pull function),
  // and optionally returns a value.
  // This type is mostly useful for method chaining.
  template <typename TReturn, typename T>
  using sink_fn = std::function<TReturn(source_fn<T>)>;

  // A sink function that doesn't return a value.
  template <typename T>
  using cap_fn = sink_fn<void, T>;

  // A sink function that is also (or rather, returns) a source function!
  // The signature for its factory (which is called an operator) usually looks like this:
  //
  //   (params) => (source_fn) => (push_fn, end_fn) => (pull_fn)
  template <typename TOut, typename TIn>
  using pipe_fn = sink_fn<source_fn<TOut>, TIn>;

  // Some types for common functional operators.
  template <typename TOut, typename TIn>
  using map_fn = std::function<TOut(TIn)>;

  // Does what it says on the tin.
  // Some mapping functions need context.
  template <typename TOut, typename TIn, typename TContext>
  using map_with_context_fn = std::function<TOut(TIn, TContext)>;

  template <typename T>
  using reduce_fn = map_with_context_fn<T, T, T>;

  template <typename TAcc, typename TIn>
  using fold_fn = map_with_context_fn<TAcc, TAcc, TIn>;

  template <typename T>
  using filter_fn = std::function<bool(T)>;

  template <typename T>
  using exec_fn = std::function<void(T)>;

  template <typename TCombined, typename T1, typename T2>
  using combine2_fn = std::function<TCombined(T1, T2)>;

  template <typename TCombined, typename T1, typename T2, typename T3>
  using combine3_fn = std::function<TCombined(T1, T2, T3)>;

  template <typename TCombined, typename T1, typename T2, typename T3, typename T4>
  using combine4_fn = std::function<TCombined(T1, T2, T3, T4)>;

  // Shorthands for different kinds of time:
  // monotonic and wall time, which are synonyms for
  // steady time and system time.
  using mono_duration = std::chrono::steady_clock::duration;
  using mono_time_point = std::chrono::steady_clock::time_point;
  using wall_duration = std::chrono::system_clock::duration;
  using wall_time_point = std::chrono::system_clock::time_point;

  // Can be used to represent the state of input switches that are pulled high or low,
  // or to represent commands to drive switch outputs.
  enum class SwitchState {
    off,
    on,
  };

  // Can be used to represent the state of openable inputs such as doors or valves,
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

}