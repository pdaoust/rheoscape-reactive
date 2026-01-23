#pragma once

#include <chrono>
#include <concepts>
#include <functional>
#include <type_traits>

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

  // Ergonomic chaining of source and sink using the `|` operator.
  // This lets you go:
  //
  //   let stringified_squared_stream = constant(3)
  //     | map([](int v) { return v * v; })
  //     | filter([](int v) { return v > 5; }); // always true for a stream of nines
  //     | map([](int v) { return fmt::format("{}", v); })
  //     | foreach([](std::string v) { std::cout << v << std::endl; });
  template <typename SinkFn, typename TIn>
  auto operator|(source_fn<TIn> left, SinkFn&& right)
  -> decltype(std::declval<SinkFn>()(std::declval<source_fn<TIn>>())) {
    return right(left);
  }

  // Overload for better casting of pipe function output.
  template <typename TOut, typename TIn>
  auto operator|(source_fn<TIn> left, pipe_fn<TOut, TIn> right)
  -> source_fn<TOut> {
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

  // Type traits to detect std::chrono types.
  template<typename T>
  struct is_chrono_duration : std::false_type {};

  template<typename Rep, typename Period>
  struct is_chrono_duration<std::chrono::duration<Rep, Period>> : std::true_type {};

  template<typename T>
  inline constexpr bool is_chrono_duration_v = is_chrono_duration<T>::value;

  template<typename T>
  struct is_chrono_time_point : std::false_type {};

  template<typename Clock, typename Duration>
  struct is_chrono_time_point<std::chrono::time_point<Clock, Duration>> : std::true_type {};

  template<typename T>
  inline constexpr bool is_chrono_time_point_v = is_chrono_time_point<T>::value;

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

  // ============================================================================
  // C++20 Concepts for Type-Safe Callables
  // ============================================================================
  //
  // These concepts provide compile-time validation of callable types.
  // They work alongside std::invoke_result_t for type deduction.

  // Helper to check if a type is std::optional<T> for some T
  template<typename T>
  struct is_optional : std::false_type {};

  template<typename T>
  struct is_optional<std::optional<T>> : std::true_type {};

  template<typename T>
  inline constexpr bool is_optional_v = is_optional<T>::value;

  namespace concepts {

    // Base callable concept - anything invocable with given arguments
    template<typename F, typename... Args>
    concept Callable = std::invocable<F, Args...>;

    // Transformer: Single-input function that returns non-void
    template<typename F, typename TIn>
    concept Transformer = requires(F f, TIn in) {
      { f(in) } -> std::convertible_to<std::invoke_result_t<F, TIn>>;
      requires !std::is_void_v<std::invoke_result_t<F, TIn>>;
    };

    // Visitor: Single-input function that returns void
    template<typename F, typename TIn>
    concept Visitor = requires(F f, TIn in) {
      { f(in) } -> std::same_as<void>;
    };

    // Predicate: Single-input function that returns bool (alias for clarity)
    template<typename F, typename T>
    concept Predicate = std::predicate<F, T>;

    // Combiner2: Two-input transformer
    template<typename F, typename T1, typename T2>
    concept Combiner2 = requires(F f, T1 t1, T2 t2) {
      { f(t1, t2) } -> std::convertible_to<std::invoke_result_t<F, T1, T2>>;
      requires !std::is_void_v<std::invoke_result_t<F, T1, T2>>;
    };

    // Combiner: Variadic combiner for N inputs (N >= 2)
    template<typename F, typename... TInputs>
    concept Combiner =
      std::invocable<F, TInputs...> &&
      !std::is_void_v<std::invoke_result_t<F, TInputs...>> &&
      sizeof...(TInputs) >= 2;

    // Scanner: Accumulator function that takes (accumulator, value) and returns new accumulator
    template<typename F, typename TAcc, typename TIn>
    concept Scanner =
      std::invocable<F, TAcc, TIn> &&
      std::convertible_to<std::invoke_result_t<F, TAcc, TIn>, TAcc>;

    // FilterMapper: Function that returns std::optional<T> (for filter_map operations)
    template<typename F, typename TIn>
    concept FilterMapper =
      std::invocable<F, TIn> &&
      is_optional_v<std::invoke_result_t<F, TIn>>;

    // TimePointAndDurationCompatible: Checks that TTimePoint can be subtracted
    // to produce a result comparable with TDuration.
    // This is the fundamental relationship needed for time-based operators like
    // debounce, throttle, timed_latch, etc.
    template <typename TTimePoint, typename TDuration>
    concept TimePointAndDurationCompatible = requires(TTimePoint t1, TTimePoint t2, TDuration d) {
      { (t1 - t2) >= d } -> std::convertible_to<bool>;
    };

  } // namespace concepts

  // ============================================================================
  // Inline Control Macros for Debug/Release Builds
  // ============================================================================
  //
  // These macros control inlining behavior based on NDEBUG:
  // - Debug (NDEBUG undefined): Preserve stack frames for debugging
  // - Release (NDEBUG defined): Aggressive inlining for performance

#ifdef NDEBUG
  // Release build: suggest aggressive inlining
  #if defined(__GNUC__) || defined(__clang__)
    #define RHEO_INLINE [[gnu::always_inline]] inline
    #define RHEO_NOINLINE
  #elif defined(_MSC_VER)
    #define RHEO_INLINE __forceinline
    #define RHEO_NOINLINE
  #else
    #define RHEO_INLINE inline
    #define RHEO_NOINLINE
  #endif
#else
  // Debug build: preserve stack frames for better debugging
  #if defined(__GNUC__) || defined(__clang__)
    #define RHEO_INLINE inline
    #define RHEO_NOINLINE [[gnu::noinline]]
  #elif defined(_MSC_VER)
    #define RHEO_INLINE inline
    #define RHEO_NOINLINE __declspec(noinline)
  #else
    #define RHEO_INLINE inline
    #define RHEO_NOINLINE
  #endif
#endif

  // ============================================================================
  // Callable Type Traits
  // ============================================================================
  //
  // A single variadic trait that extracts return type and argument types
  // from any callable (lambda, function pointer, std::function, etc.)

  // Primary template - delegates to operator() inspection for callable objects
  template<typename F, typename = void>
  struct callable_traits : callable_traits<decltype(&std::decay_t<F>::operator())> {};

  // Specialization for const member function pointers (most lambdas)
  template<typename C, typename R, typename... Args>
  struct callable_traits<R(C::*)(Args...) const, void> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
    template<size_t N> using arg = std::tuple_element_t<N, args_tuple>;
  };

  // Specialization for non-const member function pointers (mutable lambdas)
  template<typename C, typename R, typename... Args>
  struct callable_traits<R(C::*)(Args...), void> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
    template<size_t N> using arg = std::tuple_element_t<N, args_tuple>;
  };

  // Specialization for function pointers
  template<typename R, typename... Args>
  struct callable_traits<R(*)(Args...), void> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
    template<size_t N> using arg = std::tuple_element_t<N, args_tuple>;
  };

  // Specialization for function types
  template<typename R, typename... Args>
  struct callable_traits<R(Args...), void> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
    template<size_t N> using arg = std::tuple_element_t<N, args_tuple>;
  };

  // Specialization for std::function
  template<typename R, typename... Args>
  struct callable_traits<std::function<R(Args...)>, void> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
    template<size_t N> using arg = std::tuple_element_t<N, args_tuple>;
  };

  // Convenience aliases
  template<typename F>
  using return_of = typename callable_traits<std::decay_t<F>>::return_type;

  template<typename F, size_t N = 0>
  using arg_of = typename callable_traits<std::decay_t<F>>::template arg<N>;

  template<typename F>
  inline constexpr size_t arity_of = callable_traits<std::decay_t<F>>::arity;
}