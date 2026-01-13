#pragma once

#include <functional>
#include <variant>
#include <core_types.hpp>
#include <Fallible.hpp>
#include <operators/filter.hpp>
#include <operators/map.hpp>
#include <types/TaggedValue.hpp>
#include <types/Wrapper.hpp>

namespace rheo::operators {

  // Lift a pipe function to a higher-ordered type;
  // e.g., a pipe_fn<TIn, TOut> to a pipe<std::optional<TIn>, std::optional<TOut>>.
  // From here on in, the lower-ordered pipe function is the 'inner' pipe,
  // and the higher-ordered upstream source (and the pipe that this function creates)
  // are the 'outer'.
  //
  // A few things are true about this utility function:
  //
  // * You need to tell `lift` how to lower a higher-ordered value
  //   and lift a lower-ordered value.
  // * Depending on the higher-ordered type, you can't always lower a value
  //   in order to pass it to the inner pipe.
  //   For instance, you can't feed `std::nullopt` into a pipe that expects real values.
  //   That means your lower_fn must return a variant
  //   of either the lowered value (position 0) if it can be lowered
  //   or the original higher-ordered value transformed into an appropriate value of TLiftedOut (position 1) if it can't.
  // * If your lower_fn can't map to a lower-order value,
  //   the transformed higher-order TLiftedOut you return will bypass the inner pipe
  //   and get pushed directly to the downstream source function.
  // * In order to lift a lower-ordered value back to a higher-ordered value,
  //   you might need to know something about the original higher-ordered value
  //   before it was sent through the inner pipe.
  //   An example would be timestamped values that are sent through an inner pipe
  //   that knows nothing about timestamps.
  //   Therefore, your lift_fn receives the original higher-ordered value
  //   as its second parameter.
  // * Either the upstream source function or the inner pipe can end the stream.
  //   Whichever happens first.
  // * Pulling the downstream source pulls all the way up the pipeline
  //   through the inner pipe to the upstream source
  //   (as long as the inner pipe pulls from its upstream source).
  // * As long as you can transform both the higher-ordered upstream values
  //   and the lower-ordered inner piped values into values of type TLiftedOut,
  //   I suppose the higher-ordered type that contains them could be totally different.
  //   Like, for instance, upstream is a `std::tuple<TIn, ?>` and downstream is a `std::optional<TOut>`.
  //   No reason you couldn't.
  //   But it'd be weird.
  //   It's not what this function is made for.

  // Push handler for outer source values - lowers and routes them
  template <typename TIn, typename TLiftedIn, typename TLiftedOut, typename LowerFn>
  struct lift_outer_push_handler {
    LowerFn lower_fn;
    push_fn<TLiftedOut> push_outer_out;
    std::shared_ptr<Wrapper<TLiftedIn>> last_lifted_in;
    push_fn<TIn> push_inner_in;

    RHEO_NOINLINE void operator()(TLiftedIn outer_value_in) const {
      last_lifted_in->value = outer_value_in;
      std::variant<TIn, TLiftedOut> maybe_lowered_value = lower_fn(outer_value_in);
      if (maybe_lowered_value.index() == 0) {
        // This value could be lowered; pass it to the inner pipe.
        push_inner_in(std::get<0>(maybe_lowered_value));
      } else {
        // This value couldn't be lowered; pass it to the outer end of the lifted pipe.
        push_outer_out(std::get<1>(maybe_lowered_value));
      }
    }
  };

  // Source binder for inner pipe input - wires outer source to inner pipe
  template <typename TIn, typename TLiftedIn, typename TLiftedOut, typename LowerFn>
  struct lift_inner_in_source_binder {
    LowerFn lower_fn;
    push_fn<TLiftedOut> push_outer_out;
    source_fn<TLiftedIn> outer_source_in;
    std::shared_ptr<Wrapper<TLiftedIn>> last_lifted_in;

    RHEO_NOINLINE pull_fn operator()(push_fn<TIn> push_inner_in) const {
      return outer_source_in(lift_outer_push_handler<TIn, TLiftedIn, TLiftedOut, LowerFn>{
        lower_fn,
        push_outer_out,
        last_lifted_in,
        std::move(push_inner_in)
      });
    }
  };

  // Push handler for inner pipe output - lifts values back up
  template <typename TOut, typename TLiftedIn, typename TLiftedOut, typename LiftFn>
  struct lift_inner_out_push_handler {
    LiftFn lift_fn;
    push_fn<TLiftedOut> push_outer_out;
    std::shared_ptr<Wrapper<TLiftedIn>> last_lifted_in;

    RHEO_NOINLINE void operator()(TOut inner_value_out) const {
      push_outer_out(lift_fn(inner_value_out, last_lifted_in->value));
    }
  };

  // Source binder for outer lifted source
  template <typename TOut, typename TIn, typename TLiftedIn, typename TLiftedOut, typename LiftFn, typename LowerFn>
  struct lift_source_binder {
    pipe_fn<TOut, TIn> inner_pipe_fn;
    LiftFn lift_fn;
    LowerFn lower_fn;
    source_fn<TLiftedIn> outer_source_in;

    RHEO_NOINLINE pull_fn operator()(push_fn<TLiftedOut> push_outer_out) const {
      auto last_lifted_in = make_wrapper_shared<TLiftedIn>();

      // Create the inner source by applying inner pipe to our custom source binder
      auto inner_source_out = inner_pipe_fn(lift_inner_in_source_binder<TIn, TLiftedIn, TLiftedOut, LowerFn>{
        lower_fn,
        push_outer_out,
        outer_source_in,
        last_lifted_in
      });

      // The pull function comes from the 'out' end of the inner pipe,
      // which is wired up all the way through to the outer source.
      return inner_source_out(lift_inner_out_push_handler<TOut, TLiftedIn, TLiftedOut, LiftFn>{
        lift_fn,
        push_outer_out,
        last_lifted_in
      });
    }
  };

  // Pipe factory for lift
  template <typename TOut, typename TIn, typename TLiftedIn, typename TLiftedOut, typename LiftFn, typename LowerFn>
  struct lift_pipe_factory {
    pipe_fn<TOut, TIn> inner_pipe_fn;
    LiftFn lift_fn;
    LowerFn lower_fn;

    RHEO_NOINLINE source_fn<TLiftedOut> operator()(source_fn<TLiftedIn> outer_source_in) const {
      return lift_source_binder<TOut, TIn, TLiftedIn, TLiftedOut, LiftFn, LowerFn>{
        inner_pipe_fn,
        lift_fn,
        lower_fn,
        std::move(outer_source_in)
      };
    }
  };

  template <typename TOut, typename TIn, typename LiftFn, typename LowerFn>
  auto lift(
    pipe_fn<TOut, TIn> inner_pipe_fn,
    LiftFn lift_fn,
    LowerFn lower_fn
  )
  -> pipe_fn<
    return_of<LiftFn>,
    arg_of<LowerFn>
  > {
    using TLiftedOut = return_of<LiftFn>;
    using TLiftedIn = arg_of<LowerFn>;

    return lift_pipe_factory<TOut, TIn, TLiftedIn, TLiftedOut, LiftFn, LowerFn>{
      std::move(inner_pipe_fn),
      std::move(lift_fn),
      std::move(lower_fn)
    };
  }

  template <typename TOut, typename TIn>
  auto lift_to_optional(pipe_fn<TOut, TIn> inner_pipe_fn)
  -> pipe_fn<std::optional<TOut>, std::optional<TIn>> {
    return lift(
      inner_pipe_fn,
      [](TOut value, std::optional<TIn> _) { return std::optional<TOut>(value); },
      [](std::optional<TIn> value) {
        return value.has_value()
          ? (std::variant<TIn, std::optional<TOut>>)value.value()
          : (std::variant<TIn, std::optional<TOut>>)std::nullopt;
      }
    );
  }

  template <typename TTag, typename TOut, typename TIn>
  auto lift_to_tagged_value(pipe_fn<TOut, TIn> inner_pipe_fn)
  -> pipe_fn<TaggedValue<TOut, TTag>, TaggedValue<TIn, TTag>> {
    return lift(
      inner_pipe_fn,
      [](TOut value, TaggedValue<TIn, TTag> tagged_in) { return TaggedValue<TOut, TTag>{ value, tagged_in.tag }; },
      [](TaggedValue<TIn, TTag> value) { return (std::variant<TIn, TaggedValue<TOut, TTag>>)value.value; }
    );
  }

  template <typename TErr, typename TOut, typename TIn>
  auto lift_to_fallible(pipe_fn<TOut, TIn> inner_pipe_fn)
  -> pipe_fn<Fallible<TOut, TErr>, Fallible<TIn, TErr>> {
    return lift(
      inner_pipe_fn,
      [](TOut value, Fallible<TIn, TErr> fallible_in) { return Fallible<TOut, TErr>{ value, fallible_in.error() }; },
      [](Fallible<TIn, TErr> value) {
        return value.is_ok()
          ? (std::variant<TIn, Fallible<TErr, TOut>>)value.value()
          : (std::variant<TIn, Fallible<TErr, TOut>>)value;
      }
    );
  }

  template <typename TRight, typename TOut, typename TIn>
  auto lift_to_tuple_left(pipe_fn<TOut, TIn> inner_pipe_fn)
  -> pipe_fn<std::tuple<TOut, TRight>, std::tuple<TIn, TRight>> {
    return lift(
      inner_pipe_fn,
      [](TOut value, std::tuple<TIn, TRight> tuple_in) { return std::tuple<TOut, TRight>{ value, std::get<1>(tuple_in) }; },
      [](std::tuple<TIn, TRight> value) { return (std::variant<TIn, std::tuple<TOut, TRight>>)value; }
    );
  }

  template <typename TLeft, typename TOut, typename TIn>
  auto lift_to_tuple_right(pipe_fn<TOut, TIn> inner_pipe_fn)
  -> pipe_fn<std::tuple<TLeft, TOut>, std::tuple<TLeft, TIn>> {
    return lift(
      inner_pipe_fn,
      [](TOut value, std::tuple<TLeft, TIn> tuple_in) { return std::tuple<TLeft, TOut>{ std::get<0>(tuple_in), value }; },
      [](std::tuple<TLeft, TIn> value) { return (std::variant<TIn, std::tuple<TLeft, TOut>>)value; }
    );
  }

}