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
  template <typename TOut, typename TIn, typename LiftFn, typename LowerFn>
  auto lift(
    // The pipe function to wrap in a higher-ordered pipe.
    pipe_fn<TOut, TIn> innerPipeFn,
    // A mapper that maps from values of the downstream end of the inner pipe
    // to the higher-ordered values of the downstream end of the outer pipe.
    // It must receive the original upstream value as context,
    // but it can feel free to ignore it.
    LiftFn&& liftFn,
    // A mapper that maps from values of the upstream end of the outer pipe
    // to values of the upstream end of the inner pipe.
    // If it can't map (e.g., std::nullopt<int> -> int)
    // it must transform into a value of the downstream end of the outer pipe.
    // This function must return its values as a std::variant<TIn, TOuterOut>
    // where TOuterOut is the same as the return type of the liftFn
    // (that is, it's the output type of the outer pipe we're building).
    LowerFn&& lowerFn
  )
  -> pipe_fn<
    transformer_2_in_out_type_t<std::decay_t<LiftFn>>,
    transformer_1_in_in_type_t<std::decay_t<LowerFn>>
  > {
    using TLiftedOut = transformer_2_in_out_type_t<std::decay_t<LiftFn>>;
    using TLiftedIn = transformer_1_in_in_type_t<std::decay_t<LowerFn>>;

    // My profound apologies to anyone reading this code,
    // including my future self.
    // It shows the non-intuitiveness of callbacks.
    // I don't even quite understand how it works;
    // all I know is that I found the magic formula that makes tests pass.
    return [innerPipeFn, liftFn = std::forward<LiftFn>(liftFn), lowerFn = std::forward<LowerFn>(lowerFn)](source_fn<TLiftedIn> outerSourceIn) {
      return [innerPipeFn, liftFn, lowerFn, outerSourceIn](push_fn<TLiftedOut> pushOuterOut) {
        auto lastLiftedIn = make_wrapper_shared<TLiftedIn>();

        auto innerSourceOut = innerPipeFn([lowerFn, pushOuterOut, outerSourceIn, lastLiftedIn](push_fn<TIn> pushInnerIn) {
          // The pull function for the 'in' end of the inner pipe
          // comes from the outer source.
          return outerSourceIn(
            [lowerFn, pushOuterOut, lastLiftedIn, pushInnerIn](TLiftedIn outerValueIn) {
              lastLiftedIn->value = outerValueIn;
              std::variant<TIn, TLiftedOut> maybeLoweredValue = lowerFn(outerValueIn);
              if (maybeLoweredValue.index() == 0) {
                // This value could be lowered; pass it to the inner pipe.
                pushInnerIn(std::get<0>(maybeLoweredValue));
              } else {
                // This value couldn't be lowered; pass it to the outer end of the lifted pipe.
                pushOuterOut(std::get<1>(maybeLoweredValue));
              }
            });
          }
        );

        // The pull function that we return comes from the 'out' end of the inner pipe,
        // which is wired up all the way through to the outer source.
        return innerSourceOut([liftFn, pushOuterOut, lastLiftedIn](TOut innerValueOut) {
          pushOuterOut(liftFn(innerValueOut, lastLiftedIn->value));
        });
      };
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