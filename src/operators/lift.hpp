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
  //   That means your lowerFn must return a variant
  //   of either the lowered value (position 0) if it can be lowered
  //   or the original higher-ordered value transformed into an appropriate value of TLiftedOut (position 1) if it can't.
  // * If your lowerFn can't map to a lower-order value,
  //   the transformed higher-order TLiftedOut you return will bypass the inner pipe
  //   and get pushed directly to the downstream source function.
  // * In order to lift a lower-ordered value back to a higher-ordered value,
  //   you might need to know something about the original higher-ordered value
  //   before it was sent through the inner pipe.
  //   An example would be timestamped values that are sent through an inner pipe
  //   that knows nothing about timestamps.
  //   Therefore, your liftFn receives the original higher-ordered value
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
    LowerFn lowerFn;
    push_fn<TLiftedOut> pushOuterOut;
    std::shared_ptr<Wrapper<TLiftedIn>> lastLiftedIn;
    push_fn<TIn> pushInnerIn;

    RHEO_NOINLINE void operator()(TLiftedIn outerValueIn) const {
      lastLiftedIn->value = outerValueIn;
      std::variant<TIn, TLiftedOut> maybeLoweredValue = lowerFn(outerValueIn);
      if (maybeLoweredValue.index() == 0) {
        // This value could be lowered; pass it to the inner pipe.
        pushInnerIn(std::get<0>(maybeLoweredValue));
      } else {
        // This value couldn't be lowered; pass it to the outer end of the lifted pipe.
        pushOuterOut(std::get<1>(maybeLoweredValue));
      }
    }
  };

  // Source binder for inner pipe input - wires outer source to inner pipe
  template <typename TIn, typename TLiftedIn, typename TLiftedOut, typename LowerFn>
  struct lift_inner_in_source_binder {
    LowerFn lowerFn;
    push_fn<TLiftedOut> pushOuterOut;
    source_fn<TLiftedIn> outerSourceIn;
    std::shared_ptr<Wrapper<TLiftedIn>> lastLiftedIn;

    RHEO_NOINLINE pull_fn operator()(push_fn<TIn> pushInnerIn) const {
      return outerSourceIn(lift_outer_push_handler<TIn, TLiftedIn, TLiftedOut, LowerFn>{
        lowerFn,
        pushOuterOut,
        lastLiftedIn,
        std::move(pushInnerIn)
      });
    }
  };

  // Push handler for inner pipe output - lifts values back up
  template <typename TOut, typename TLiftedIn, typename TLiftedOut, typename LiftFn>
  struct lift_inner_out_push_handler {
    LiftFn liftFn;
    push_fn<TLiftedOut> pushOuterOut;
    std::shared_ptr<Wrapper<TLiftedIn>> lastLiftedIn;

    RHEO_NOINLINE void operator()(TOut innerValueOut) const {
      pushOuterOut(liftFn(innerValueOut, lastLiftedIn->value));
    }
  };

  // Source binder for outer lifted source
  template <typename TOut, typename TIn, typename TLiftedIn, typename TLiftedOut, typename LiftFn, typename LowerFn>
  struct lift_source_binder {
    pipe_fn<TOut, TIn> innerPipeFn;
    LiftFn liftFn;
    LowerFn lowerFn;
    source_fn<TLiftedIn> outerSourceIn;

    RHEO_NOINLINE pull_fn operator()(push_fn<TLiftedOut> pushOuterOut) const {
      auto lastLiftedIn = make_wrapper_shared<TLiftedIn>();

      // Create the inner source by applying inner pipe to our custom source binder
      auto innerSourceOut = innerPipeFn(lift_inner_in_source_binder<TIn, TLiftedIn, TLiftedOut, LowerFn>{
        lowerFn,
        pushOuterOut,
        outerSourceIn,
        lastLiftedIn
      });

      // The pull function comes from the 'out' end of the inner pipe,
      // which is wired up all the way through to the outer source.
      return innerSourceOut(lift_inner_out_push_handler<TOut, TLiftedIn, TLiftedOut, LiftFn>{
        liftFn,
        pushOuterOut,
        lastLiftedIn
      });
    }
  };

  // Pipe factory for lift
  template <typename TOut, typename TIn, typename TLiftedIn, typename TLiftedOut, typename LiftFn, typename LowerFn>
  struct lift_pipe_factory {
    pipe_fn<TOut, TIn> innerPipeFn;
    LiftFn liftFn;
    LowerFn lowerFn;

    RHEO_NOINLINE source_fn<TLiftedOut> operator()(source_fn<TLiftedIn> outerSourceIn) const {
      return lift_source_binder<TOut, TIn, TLiftedIn, TLiftedOut, LiftFn, LowerFn>{
        innerPipeFn,
        liftFn,
        lowerFn,
        std::move(outerSourceIn)
      };
    }
  };

  template <typename TOut, typename TIn, typename LiftFn, typename LowerFn>
  auto lift(
    pipe_fn<TOut, TIn> innerPipeFn,
    LiftFn liftFn,
    LowerFn lowerFn
  )
  -> pipe_fn<
    return_of<LiftFn>,
    arg_of<LowerFn>
  > {
    using TLiftedOut = return_of<LiftFn>;
    using TLiftedIn = arg_of<LowerFn>;

    return lift_pipe_factory<TOut, TIn, TLiftedIn, TLiftedOut, LiftFn, LowerFn>{
      std::move(innerPipeFn),
      std::move(liftFn),
      std::move(lowerFn)
    };
  }

  template <typename TOut, typename TIn>
  auto liftToOptional(pipe_fn<TOut, TIn> innerPipeFn)
  -> pipe_fn<std::optional<TOut>, std::optional<TIn>> {
    return lift(
      innerPipeFn,
      [](TOut value, std::optional<TIn> _) { return std::optional<TOut>(value); },
      [](std::optional<TIn> value) {
        return value.has_value()
          ? (std::variant<TIn, std::optional<TOut>>)value.value()
          : (std::variant<TIn, std::optional<TOut>>)std::nullopt;
      }
    );
  }

  template <typename TTag, typename TOut, typename TIn>
  auto liftToTaggedValue(pipe_fn<TOut, TIn> innerPipeFn)
  -> pipe_fn<TaggedValue<TOut, TTag>, TaggedValue<TIn, TTag>> {
    return lift(
      innerPipeFn,
      [](TOut value, TaggedValue<TIn, TTag> taggedIn) { return TaggedValue<TOut, TTag>{ value, taggedIn.tag }; },
      [](TaggedValue<TIn, TTag> value) { return (std::variant<TIn, TaggedValue<TOut, TTag>>)value.value; }
    );
  }

  template <typename TErr, typename TOut, typename TIn>
  auto liftToFallible(pipe_fn<TOut, TIn> innerPipeFn)
  -> pipe_fn<Fallible<TOut, TErr>, Fallible<TIn, TErr>> {
    return lift(
      innerPipeFn,
      [](TOut value, Fallible<TIn, TErr> fallibleIn) { return Fallible<TOut, TErr>{ value, fallibleIn.error() }; },
      [](Fallible<TIn, TErr> value) {
        return value.isOk()
          ? (std::variant<TIn, Fallible<TErr, TOut>>)value.value()
          : (std::variant<TIn, Fallible<TErr, TOut>>)value;
      }
    );
  }

  template <typename TRight, typename TOut, typename TIn>
  auto liftToTupleLeft(pipe_fn<TOut, TIn> innerPipeFn)
  -> pipe_fn<std::tuple<TOut, TRight>, std::tuple<TIn, TRight>> {
    return lift(
      innerPipeFn,
      [](TOut value, std::tuple<TIn, TRight> tupleIn) { return std::tuple<TOut, TRight>{ value, std::get<1>(tupleIn) }; },
      [](std::tuple<TIn, TRight> value) { return (std::variant<TIn, std::tuple<TOut, TRight>>)value; }
    );
  }

  template <typename TLeft, typename TOut, typename TIn>
  auto liftToTupleRight(pipe_fn<TOut, TIn> innerPipeFn)
  -> pipe_fn<std::tuple<TLeft, TOut>, std::tuple<TLeft, TIn>> {
    return lift(
      innerPipeFn,
      [](TOut value, std::tuple<TLeft, TIn> tupleIn) { return std::tuple<TLeft, TOut>{ std::get<0>(tupleIn), value }; },
      [](std::tuple<TLeft, TIn> value) { return (std::variant<TIn, std::tuple<TLeft, TOut>>)value; }
    );
  }
  
}