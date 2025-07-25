#pragma once

#include <chrono>
#include <functional>

namespace rheo {

  // Rheoscape is not a library.
  // It's just a specification for hybrid reactive/pullable iterables
  // and how they should work.
  // Every source is a function that takes a `push` callback and an `end` callback,
  // and returns a `pull` callback.
  //
  // A consumer, called a sink, figures out what it wants to do when a source pushes to it,
  // figures out what it wants to do when a source ends,
  // and then calls a source function with callbacks that embody that logic.
  //
  // A sink is just a pair of functions --
  // a callback to receive a pushed value,
  // and a callback to handle the ending of a source.
  // When a source function is called with a sink,
  // the caller receives a function that it can then use
  // to pull new data if it likes
  // (and if the source function supports pulling).
  //
  // The source function can be called many times,
  // each time binding to a new sink,
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

  // Helper function to convert any callable to std::function for analysis.
  template<typename F>
  auto to_std_function(F&& f) {
    return std::function{std::forward<F>(f)};  // C++17 deduction guides work magic here.
  }

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

  const pull_fn empty_pull_fn = [](){};

  // A function that a sink provides to a source
  // to allow the source to signal that there won't be any more values.
  using end_fn = signal_fn;

  const end_fn empty_end_fn = [](){};

  // A function that produces values and pushes them to a sink.
  // It needs to receive a push function and an end function from the sink,
  // and returns a pull function to the sink.
  // It's usually created inside a factory that has a structure like this:
  //
  //   (params) => (push_fn) => (pull_fn)
  template <typename T>
  using source_fn = std::function<pull_fn(push_fn<T>)>;

  // Helper trait to extract the value type from a source_fn.
  template<typename T>
  struct source_value_type {
    static_assert(std::is_same_v<T, void>, "type must be a source function");
  };

  template<typename T>
  struct source_value_type<source_fn<T>> {
      using type = T;
  };

  template<typename T>
  using source_value_type_t = typename source_value_type<T>::type;

  // A function that receives a source function,
  // binds itself to the source
  // (that is, passes it a push function that handles values pushed to it
  // and optionally uses its pull function),
  // and optionally returns a value.
  // This type is mostly useful for method chaining.
  template <typename TReturn, typename T>
  using sink_fn = std::function<TReturn(source_fn<T>)>;

  template <typename T>
  struct sink_value_types {
    static_assert(std::is_same_v<T, void>, "type must be a sink function");
  };

  template <typename T, typename R>
  struct sink_value_types<sink_fn<R, T>> {
    using source_type = T;
    using return_type = R;
  };

  template <typename T, typename R>
  struct sink_value_types<R(*)(source_fn<T>)> {
    using source_type = T;
    using return_type = R;
  };

  template <typename T, typename R>
  struct sink_value_types<R(source_fn<T>)> {
    using source_type = T;
    using return_type = R;
  };

  template <typename F>
  using sink_value_types_from_callable = sink_value_types<decltype(to_std_function(std::declval<F>()))>;

  template <typename T>
  using sink_source_type_t = typename sink_value_types_from_callable<T>::source_type;

  template <typename T>
  using sink_return_type_t = typename sink_value_types_from_callable<T>::return_type;

  // A sink function that doesn't return a value.
  template <typename T>
  using cap_fn = sink_fn<void, T>;

  // A sink function that can be pulled --
  // that is, when bound to a source function,
  // it returns a pull function to pull from the source.
  template <typename T>
  using pullable_sink_fn = sink_fn<pull_fn, T>;

  // A sink function that is also (or rather, returns) a source function!
  // The signature for its factory (which is called an operator) usually looks like this:
  //
  //   (params) => (source_fn) => (push_fn, end_fn) => (pull_fn)
  template <typename TOut, typename TIn>
  using pipe_fn = sink_fn<source_fn<TOut>, TIn>;

  // Ergonomic chaining of pipe functions using the `|` operator.
  // This lets you go:
  //
  //   let stringifiedSquaredStream = constant(3)
  //     | map([](int v) { return v * v; })
  //     | filter([](int v) { return v > 5; }); // always true for a stream of nines
  //     | map([](int v) { return fmt::format("{}", v); });
  template<typename TOut, typename TIn>
  source_fn<TOut> operator|(source_fn<TIn> left, pipe_fn<TOut, TIn> right) {
    return right(left);
  }

  // Some types for common functional operators.
  template <typename TOut, typename TIn>
  using map_fn = std::function<TOut(TIn)>;

  template <typename TOut, typename TIn>
  using filter_map_fn = std::function<std::optional<TOut>(TIn)>;

  // Does what it says on the tin.
  // Some mapping functions need context.
  template <typename TOut, typename TIn, typename TContext>
  using map_with_context_fn = std::function<TOut(TIn, TContext)>;

  template <typename T>
  using reduce_fn = std::function<T(T, T)>;

  template <typename TAcc, typename TIn>
  using fold_fn = std::function<TAcc(TAcc, TIn)>;

  template <typename T>
  using filter_fn = std::function<bool(T)>;

  template <typename TOut, typename TIn>
  using filter_map_fn = std::function<std::optional<TOut>(TIn)>;

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

  // Type deduction helper traits for functions of various sizes.

  // One parameter and void return value.
  template <typename T>
  struct visitor_1_in_value_type {
    static_assert(std::is_same_v<T, void>, "type must be a 1-parameter visitor function");
  };

  template <typename TIn>
  struct visitor_1_in_value_type<std::function<void(TIn)>> {
    using in_type = TIn;
  };

  template <typename TIn>
  struct visitor_1_in_value_type<void(*)(TIn)> {
    using in_type = TIn;
  };

  template <typename TIn>
  struct visitor_1_in_value_type<void(TIn)> {
    using in_type = TIn;
  };

  template<typename F>
  using visitor_1_in_value_type_from_callable = visitor_1_in_value_type<decltype(to_std_function(std::declval<F>()))>;

  template <typename F>
  using visitor_1_in_in_type_t = typename visitor_1_in_value_type_from_callable<F>::in_type;

  // One parameter and non-void return value.
  template <typename T>
  struct transformer_1_in_value_types {
    static_assert(std::is_same_v<T, void>, "type must be a 1-parameter transformer function");
  };

  template <typename TOut, typename TIn>
  struct transformer_1_in_value_types<std::function<TOut(TIn)>> {
    using out_type = TOut;
    using in_type = TIn;
  };

  template<typename TOut, typename TIn>
  struct transformer_1_in_value_types<TOut(*)(TIn)> {
    using out_type = TOut;
    using in_type = TIn;
  };

  template<typename F>
  using transformer_1_in_value_types_from_callable = transformer_1_in_value_types<decltype(to_std_function(std::declval<F>()))>;

  template <typename F>
  using transformer_1_in_out_type_t = typename transformer_1_in_value_types_from_callable<F>::out_type;

  template <typename F>
  using transformer_1_in_in_type_t = typename transformer_1_in_value_types_from_callable<F>::in_type;

  // Two parameters and non-void return value.
  template <typename T>
  struct transformer_2_in_value_types {
    static_assert(std::is_same_v<T, void>, "type must be a 2-parameter transformer function");
  };

  template <typename TOut, typename TIn1, typename TIn2>
  struct transformer_2_in_value_types<std::function<TOut(TIn1, TIn2)>> {
    using out_type = TOut;
    using in_1_type = TIn1;
    using in_2_type = TIn2;
  };

  template<typename TOut, typename TIn1, typename TIn2>
  struct transformer_2_in_value_types<TOut(*)(TIn1, TIn2)> {
    using out_type = TOut;
    using in_1_type = TIn1;
    using in_2_type = TIn2;
  };

  template<typename F>
  using transformer_2_in_value_types_from_callable = transformer_2_in_value_types<decltype(to_std_function(std::declval<F>()))>;

  template <typename F>
  using transformer_2_in_out_type_t = typename transformer_2_in_value_types_from_callable<F>::out_type;

  template <typename F>
  using transformer_2_in_in_1_type_t = typename transformer_2_in_value_types_from_callable<F>::in_1_type;

  template <typename F>
  using transformer_2_in_in_2_type_t = typename transformer_2_in_value_types_from_callable<F>::in_2_type;

  // Three parameters and non-void return value.
  template <typename T>
  struct transformer_3_in_value_types {
    static_assert(std::is_same_v<T, void>, "type must be a 3-parameter transformer function");
  };

  template <typename TOut, typename TIn1, typename TIn2, typename TIn3>
  struct transformer_3_in_value_types<std::function<TOut(TIn1, TIn2, TIn3)>> {
    using out_type = TOut;
    using in_1_type = TIn1;
    using in_2_type = TIn2;
    using in_3_type = TIn3;
  };

  template<typename TOut, typename TIn1, typename TIn2, typename TIn3>
  struct transformer_3_in_value_types<TOut(*)(TIn1, TIn2, TIn3)> {
    using out_type = TOut;
    using in_1_type = TIn1;
    using in_2_type = TIn2;
    using in_3_type = TIn3;
  };

  template<typename F>
  using transformer_3_in_value_types_from_callable = transformer_3_in_value_types<decltype(to_std_function(std::declval<F>()))>;

  template <typename F>
  using transformer_3_in_out_type_t = typename transformer_3_in_value_types_from_callable<F>::out_type;

  template <typename F>
  using transformer_3_in_in_1_type_t = typename transformer_3_in_value_types_from_callable<F>::in_1_type;

  template <typename F>
  using transformer_3_in_in_2_type_t = typename transformer_3_in_value_types_from_callable<F>::in_2_type;

  template <typename F>
  using transformer_3_in_in_3_type_t = typename transformer_3_in_value_types_from_callable<F>::in_3_type;

  // Four parameters and non-void return value.
  template <typename T>
  struct transformer_4_in_value_types {
    static_assert(std::is_same_v<T, void>, "type must be a 4-parameter transformer function");
  };

  template <typename TOut, typename TIn1, typename TIn2, typename TIn3, typename TIn4>
  struct transformer_4_in_value_types<std::function<TOut(TIn1, TIn2, TIn3, TIn4)>> {
    using out_type = TOut;
    using in_1_type = TIn1;
    using in_2_type = TIn2;
    using in_3_type = TIn3;
    using in_4_type = TIn4;
  };

  template<typename TOut, typename TIn1, typename TIn2, typename TIn3, typename TIn4>
  struct transformer_4_in_value_types<TOut(*)(TIn1, TIn2, TIn3, TIn4)> {
    using out_type = TOut;
    using in_1_type = TIn1;
    using in_2_type = TIn2;
    using in_3_type = TIn3;
    using in_4_type = TIn4;
  };

  template<typename F>
  using transformer_4_in_value_types_from_callable = transformer_4_in_value_types<decltype(to_std_function(std::declval<F>()))>;

  template <typename F>
  using transformer_4_in_out_type_t = typename transformer_4_in_value_types_from_callable<F>::out_type;

  template <typename F>
  using transformer_4_in_in_1_type_t = typename transformer_4_in_value_types_from_callable<F>::in_1_type;

  template <typename F>
  using transformer_4_in_in_2_type_t = typename transformer_4_in_value_types_from_callable<F>::in_2_type;

  template <typename F>
  using transformer_4_in_in_3_type_t = typename transformer_4_in_value_types_from_callable<F>::in_3_type;

  template <typename F>
  using transformer_4_in_in_4_type_t = typename transformer_4_in_value_types_from_callable<F>::in_4_type;

  // And a way of getting the wrapped type out of a filter mapper function.
  template <typename T>
  struct filter_mapper_value_type {
    static_assert(std::is_same_v<T, void>, "type must be a filter mapper function");
  };

  template <typename TOut, typename TIn>
  struct filter_mapper_value_type<std::function<std::optional<TOut>(TIn)>> {
    using wrapped_out_type = TOut;
  };

  template<typename F>
  using filter_mapper_value_type_from_callable = filter_mapper_value_type<decltype(to_std_function(std::declval<F>()))>;

  template <typename F>
  using filter_mapper_wrapped_out_type_t = typename filter_mapper_value_type_from_callable<F>::wrapped_out_type;
}