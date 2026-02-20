#pragma once

#include <chrono>
#include <concepts>
#include <functional>
#include <type_traits>

namespace rheoscape {

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

  // Helper trait to extract the value type from a source.
  //
  // Two paths:
  // 1. Structs with `using value_type = T;` (our SourceBinders)
  // 2. Callables whose first arg is push_fn<T> (lambdas, std::function)
  //
  // The primary template handles path 1 (value_type member).
  // Specializations handle path 2 (callable inspection).
  template<typename T, typename = void>
  struct source_value_type {};

  // Path 1: struct with explicit value_type member
  template<typename T>
  struct source_value_type<T, std::void_t<typename T::value_type>> {
    using type = typename T::value_type;
  };

  // Path 2: source_fn<T> (backward compat, also covers std::function)
  template<typename T>
  struct source_value_type<source_fn<T>, void> {
    using type = T;
  };

  template<typename T>
  using source_value_type_t = typename source_value_type<T>::type;

  // Convenience alias (preferred going forward)
  template<typename T>
  using source_value_t = typename source_value_type<std::decay_t<T>>::type;

  // TODO: sink_fn, cap_fn, pullable_sink_fn, and pipe_fn are type-erased
  // sink aliases that predate the concepts-based approach.
  // Now that sinks use generic SourceFn parameters constrained by
  // concepts::SourceOf, these aliases are only needed for backward
  // compatibility and could be removed once all consumers are migrated.

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

  // Forward-declare the Source concept so operator| can use it.
  // Full concept definitions are in the concepts namespace below.
  namespace concepts {
    template <typename S>
    concept Source = requires {
      typename source_value_type<std::decay_t<S>>::type;
    } && std::invocable<
      std::decay_t<S>,
      push_fn<typename source_value_type<std::decay_t<S>>::type>
    >;
  }

  // Ergonomic chaining of source and sink using the `|` operator.
  // This lets you go:
  //
  //   let stringified_squared_stream = constant(3)
  //     | map([](int v) { return v * v; })
  //     | filter([](int v) { return v > 5; }); // always true for a stream of nines
  //     | map([](int v) { return fmt::format("{}", v); })
  //     | foreach([](std::string v) { std::cout << v << std::endl; });

  // Generic overload: works with any Source (concrete SourceBinders, source_fn, etc.)
  // During migration, unconverted pipe factories accept source_fn<TIn> which can't be
  // deduced from a concrete source type. The if-constexpr fallback converts to source_fn
  // so unconverted operators keep working alongside converted ones.
  template <typename SourceT, typename SinkFn>
    requires concepts::Source<std::decay_t<SourceT>>
  decltype(auto) operator|(SourceT&& left, SinkFn&& right) {
    if constexpr (std::is_invocable_v<SinkFn&&, SourceT&&>) {
      return std::forward<SinkFn>(right)(std::forward<SourceT>(left));
    } else {
      using T = source_value_t<SourceT>;
      return std::forward<SinkFn>(right)(source_fn<T>(std::forward<SourceT>(left)));
    }
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

  // Type trait to detect std::tuple types.
  template<typename T>
  struct is_tuple : std::false_type {};
  template<typename... Ts>
  struct is_tuple<std::tuple<Ts...>> : std::true_type {};
  template<typename T>
  inline constexpr bool is_tuple_v = is_tuple<T>::value;

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

  // Helper to check if a type is std::vector<T> for some T,
  // and to extract the element type.
  template<typename T>
  struct is_vector : std::false_type {};

  template<typename T, typename Alloc>
  struct is_vector<std::vector<T, Alloc>> : std::true_type {};

  template<typename T>
  inline constexpr bool is_vector_v = is_vector<T>::value;

  // Extract the element type from a std::vector<T>.
  template<typename V>
  struct vector_value_type;

  template<typename T, typename Alloc>
  struct vector_value_type<std::vector<T, Alloc>> { using type = T; };

  template<typename V>
  using vector_value_t = typename vector_value_type<V>::type;

  // ============================================================================
  // Apply Result Type Trait
  // ============================================================================
  //
  // Helper to get the result type of applying a callable to a tuple via std::apply.

  template<typename F, typename Tuple>
  struct apply_result;

  template<typename F, typename... Args>
  struct apply_result<F, std::tuple<Args...>> {
    using type = std::invoke_result_t<F, Args...>;
  };

  template<typename F, typename Tuple>
  using apply_result_t = typename apply_result<F, Tuple>::type;

  // Prepend a type to a tuple's element list.
  // Needed for scanner fallback where we prepend the accumulator type.
  template<typename T, typename Tuple> struct tuple_prepend;
  template<typename T, typename... Ts>
  struct tuple_prepend<T, std::tuple<Ts...>> { using type = std::tuple<T, Ts...>; };
  template<typename T, typename Tuple>
  using tuple_prepend_t = typename tuple_prepend<T, Tuple>::type;

  // Result type of calling a function with either a value directly
  // or its unpacked tuple elements via std::apply.
  // Direct invocation takes priority over unpacking.
  // Uses lazy evaluation to avoid instantiating apply_result for non-tuple types.
  template<typename F, typename TIn, typename = void>
  struct invoke_maybe_apply_result {
    using type = apply_result_t<std::decay_t<F>, std::decay_t<TIn>>;
  };
  template<typename F, typename TIn>
  struct invoke_maybe_apply_result<F, TIn, std::enable_if_t<std::is_invocable_v<F, TIn>>> {
    using type = std::invoke_result_t<F, TIn>;
  };
  template<typename F, typename TIn>
  using invoke_maybe_apply_result_t = typename invoke_maybe_apply_result<F, TIn>::type;

  // Same as invoke_maybe_apply_result_t but for scanner's (acc, value) pattern.
  template<typename F, typename TAcc, typename TIn, typename = void>
  struct invoke_scanner_maybe_apply_result {
    using type = apply_result_t<std::decay_t<F>, tuple_prepend_t<TAcc, std::decay_t<TIn>>>;
  };
  template<typename F, typename TAcc, typename TIn>
  struct invoke_scanner_maybe_apply_result<F, TAcc, TIn, std::enable_if_t<std::is_invocable_v<F, TAcc, TIn>>> {
    using type = std::invoke_result_t<F, TAcc, TIn>;
  };
  template<typename F, typename TAcc, typename TIn>
  using invoke_scanner_maybe_apply_result_t = typename invoke_scanner_maybe_apply_result<F, TAcc, TIn>::type;

  // Dispatch function: tries direct call first, falls back to std::apply.
  template<typename F, typename TIn>
  decltype(auto) invoke_maybe_apply(F&& f, TIn&& value) {
    if constexpr (std::is_invocable_v<F&&, TIn&&>) {
      return std::invoke(std::forward<F>(f), std::forward<TIn>(value));
    } else {
      return std::apply(std::forward<F>(f), std::forward<TIn>(value));
    }
  }

  // Dispatch function for scanner's (acc, value) pattern.
  template<typename F, typename TAcc, typename TIn>
  decltype(auto) invoke_scanner_maybe_apply(F&& f, TAcc&& acc, TIn&& value) {
    if constexpr (std::is_invocable_v<F&&, TAcc&&, TIn&&>) {
      return std::invoke(std::forward<F>(f), std::forward<TAcc>(acc), std::forward<TIn>(value));
    } else {
      return std::apply(
        [&f, &acc](auto&&... args) -> decltype(auto) {
          return std::invoke(std::forward<F>(f), std::forward<TAcc>(acc),
                             std::forward<decltype(args)>(args)...);
        },
        std::forward<TIn>(value)
      );
    }
  }

  // Extract the output element type from a flat-map function's return vector.
  // Given F(TIn) -> std::vector<TOut>, yields TOut.
  template<typename F, typename TIn>
  using flat_map_value_t = vector_value_t<invoke_maybe_apply_result_t<F, TIn>>;

  namespace concepts {

    // Note: Source concept is defined earlier in the file (before operator|).

    // SourceOf: Convenience concept for sinks that need to constrain
    // a source to emit a specific value type.
    template <typename S, typename T>
    concept SourceOf = Source<S> && std::same_as<source_value_t<S>, T>;

    // Base callable concept - anything invocable with given arguments
    template<typename F, typename... Args>
    concept Callable = std::invocable<F, Args...>;

    // TupleMapper: A callable that can be applied to a tuple's elements via std::apply.
    // Ensures the mapper's arity matches the tuple size and returns non-void.
    // Uses std::decay_t<F> to properly handle mutable lambdas (whose operator() is non-const).
    template<typename F, typename TTuple>
    concept TupleMapper = requires(std::decay_t<F>& f, TTuple t) {
      { std::apply(f, t) };
    } && !std::is_void_v<apply_result_t<std::decay_t<F>, TTuple>>;

    // Transformer: Single-input function that returns non-void.
    // Also matches multi-arg functions when TIn is a tuple
    // (auto-unpacking via std::apply), but only if direct invocation doesn't work.
    template<typename F, typename TIn>
    concept Transformer =
      (std::invocable<F, TIn> && !std::is_void_v<std::invoke_result_t<F, TIn>>)
      || (is_tuple_v<std::decay_t<TIn>> && !std::invocable<F, TIn>
          && TupleMapper<F, std::decay_t<TIn>>);

    // Visitor: Single-input function that returns void.
    // Also matches multi-arg void functions when TIn is a tuple (auto-unpacking).
    template<typename F, typename TIn>
    concept Visitor =
      (std::invocable<F, TIn> && std::is_void_v<std::invoke_result_t<F, TIn>>)
      || (is_tuple_v<std::decay_t<TIn>> && !std::invocable<F, TIn>
          && requires(std::decay_t<F>& f, std::decay_t<TIn> t) {
            { std::apply(f, t) } -> std::same_as<void>;
          });

    // Predicate: Single-input function that returns bool.
    // Also matches multi-arg bool functions when T is a tuple (auto-unpacking).
    template<typename F, typename T>
    concept Predicate =
      std::predicate<F, T>
      || (is_tuple_v<std::decay_t<T>> && !std::invocable<F, T>
          && requires(std::decay_t<F>& f, std::decay_t<T> t) {
            { std::apply(f, t) } -> std::convertible_to<bool>;
          });

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

    // Scanner: Accumulator function that takes (accumulator, value)
    // and returns new accumulator.
    // Also matches multi-arg scanners when TIn is a tuple (auto-unpacking).
    template<typename F, typename TAcc, typename TIn>
    concept Scanner =
      (std::invocable<F, TAcc, TIn> &&
       std::convertible_to<std::invoke_result_t<F, TAcc, TIn>, TAcc>)
      || (is_tuple_v<std::decay_t<TIn>> && !std::invocable<F, TAcc, TIn>
          && std::convertible_to<
               invoke_scanner_maybe_apply_result_t<F, TAcc, TIn>, TAcc>);

    // FlatMapper: Function that takes TIn and returns std::vector<TOut> for some TOut.
    // Use flat_map_value_t<F, TIn> to extract TOut.
    // Also matches multi-arg functions when TIn is a tuple (auto-unpacking).
    template<typename F, typename TIn>
    concept FlatMapper =
      (std::invocable<F, TIn> && is_vector_v<std::invoke_result_t<F, TIn>>)
      || (is_tuple_v<std::decay_t<TIn>> && !std::invocable<F, TIn>
          && is_vector_v<apply_result_t<std::decay_t<F>, std::decay_t<TIn>>>);

    // FilterMapper: Function that returns std::optional<T> (for filter_map operations).
    // Also matches multi-arg functions when TIn is a tuple (auto-unpacking).
    template<typename F, typename TIn>
    concept FilterMapper =
      (std::invocable<F, TIn> && is_optional_v<std::invoke_result_t<F, TIn>>)
      || (is_tuple_v<std::decay_t<TIn>> && !std::invocable<F, TIn>
          && is_optional_v<apply_result_t<std::decay_t<F>, std::decay_t<TIn>>>);

    // TimePointAndDurationCompatible: Checks that TTimePoint can be subtracted
    // to produce a result comparable with TDuration.
    // This is the fundamental relationship needed for time-based operators like
    // debounce, throttle, timed_latch, etc.
    template <typename TTimePoint, typename TDuration>
    concept TimePointAndDurationCompatible = requires(TTimePoint t1, TTimePoint t2, TDuration d) {
      { (t1 - t2) >= d } -> std::convertible_to<bool>;
    };

    // Deserializable: A type that can be validated after raw deserialization
    // (e.g., from EEPROM via memcpy).
    // Scalars are always considered valid (no way to detect garbage).
    // Non-scalar types must provide `bool is_valid() const`.
    template <typename T>
    concept Deserializable = std::is_scalar_v<T> || requires(const T& t) {
      { t.is_valid() } -> std::convertible_to<bool>;
    };

  } // namespace concepts

  // Check whether a deserialized value is valid.
  // Encapsulates the two arms of the Deserializable concept
  // so callers don't need to know which arm applies.
  template <typename T>
    requires concepts::Deserializable<T>
  bool is_deserialized_valid(const T& value) {
    if constexpr (std::is_scalar_v<T>) {
      return true;
    } else {
      return value.is_valid();
    }
  }

  // ============================================================================
  // Inline Control Macro
  // ============================================================================
  //
  // RHEOSCAPE_CALLABLE controls inlining behavior for framework internals.
  // Use this on operator() methods of named callable structs.
  //
  // Configuration macros (define before including Rheoscape headers):
  //
  //   RHEOSCAPE_DEBUG_INTERNALS   - Prevent inlining so you can step through
  //                            framework code in the debugger. Useful for
  //                            Rheoscape developers or debugging operator issues.
  //
  //   RHEOSCAPE_AGGRESSIVE_INLINE - Force aggressive inlining for maximum performance
  //                            and shallow stacks. May increase binary size.
  //
  // Priority: RHEOSCAPE_DEBUG_INTERNALS > RHEOSCAPE_AGGRESSIVE_INLINE > default (inline hint)

#if defined(RHEOSCAPE_DEBUG_INTERNALS)
  // Debug internals: prevent inlining so you can step through framework code
  #if defined(__GNUC__) || defined(__clang__)
    #define RHEOSCAPE_CALLABLE [[gnu::noinline]]
  #elif defined(_MSC_VER)
    #define RHEOSCAPE_CALLABLE __declspec(noinline)
  #else
    #define RHEOSCAPE_CALLABLE
  #endif
#elif defined(RHEOSCAPE_AGGRESSIVE_INLINE)
  // Aggressive inlining: force inline for max performance and shallow stacks
  #if defined(__GNUC__) || defined(__clang__)
    #define RHEOSCAPE_CALLABLE [[gnu::always_inline]] inline
  #elif defined(_MSC_VER)
    #define RHEOSCAPE_CALLABLE __forceinline
  #else
    #define RHEOSCAPE_CALLABLE inline
  #endif
#else
  // Default: hint to compiler, let it decide based on its heuristics
  #define RHEOSCAPE_CALLABLE inline
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