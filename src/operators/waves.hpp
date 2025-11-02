#include <math.h>
#include <functional>
#include <operators/map.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>

namespace rheo::operators {

  // Calculate the ouptut of a wave function
  // given a source for the t axis (such as time)
  // and sources for the period length and phase shift.
  // Note that the phase shift is probably backwards to what you're expecting --
  // if the period is 100 units and the phase shift is 25 units,
  // this doesn't mean the wave will be shifted forward along the t axis by 25 units.
  // All you need to do, really, is reverse your thinking.
  // For example, 25 becomes 100 - 25 = 75.
  // This is to prevent integer wraparound errors
  // or errors that come from C++'s weird behaviour of giving negative modulos
  // if one operand is negative.
  template <typename TInput, typename MapFn>
  auto wave(source_fn<TInput> inputSource, source_fn<TInput> periodSource, source_fn<TInput> phaseShiftSource, MapFn&& waveFunction)
  -> source_fn<transformer_1_in_out_type_t<std::decay_t<MapFn>>> {
    using TFloat = transformer_1_in_out_type_t<std::decay_t<MapFn>>;
    
    return map(
      combine(inputSource, periodSource, phaseShiftSource, std::make_tuple<TInput, TInput, TInput>),
      [waveFunction = std::forward<MapFn>(waveFunction)](std::tuple<TInput, TInput, TInput> value) {
        TInput input = std::get<0>(value);
        TInput period = std::get<1>(value);
        TInput phaseShift = std::get<2>(value);
        TFloat inputNormalised = (TFloat)((input + phaseShift) % period) / (TFloat)period;
        return waveFunction(inputNormalised);
      }
    );
  }

  template <typename TFloat = float, typename TInput>
  source_fn<TFloat> sineWave(source_fn<TInput> inputSource, source_fn<TInput> periodSource, source_fn<TInput> phaseShiftSource) {
    return wave(inputSource, periodSource, phaseShiftSource, [](TFloat input) { return sin((TFloat)input * M_PI * 2); });
  }

  template <typename TFloat = float, typename TInput>
  source_fn<TFloat> sawtoothWave(source_fn<TInput> inputSource, source_fn<TInput> periodSource, source_fn<TInput> phaseShiftSource) {
    return wave(inputSource, periodSource, phaseShiftSource, [](TFloat input) { return input * 2 - 1; });
  }

  template <typename TFloat = float, typename TInput>
  source_fn<TFloat> triangleWave(source_fn<TInput> inputSource, source_fn<TInput> periodSource, source_fn<TInput> phaseShiftSource) {
    return wave(
      inputSource,
      periodSource,
      phaseShiftSource,
      [](TFloat input) {
        input = input * 4;
        if (input < 1) {
          return input;
        } else if (input >= 1 && input < 3) {
          return (input - 2) * -1;
        } else {
          return input - 4;
        }
      }
    );
  }

  template <typename TFloat = float, typename TInput>
  source_fn<TFloat> pwmWave(source_fn<TInput> inputSource, source_fn<TInput> periodSource, source_fn<TInput> phaseShiftSource, source_fn<TFloat> dutySource) {
    return map(
      combine(
        wave(inputSource, periodSource, phaseShiftSource, [](TFloat v) { return v; }),
        dutySource,
        std::make_tuple<TFloat, TFloat>
      ),
      [](std::tuple<TFloat, TFloat> value) {
        return std::get<0>(value) < std::get<1>(value)
            ? (TFloat)1
            : (TFloat)-1;
      }
    );
  }

  template <typename TFloat = float, typename TInput>
  source_fn<TFloat> squareWave(source_fn<TInput> inputSource, source_fn<TInput> periodSource, source_fn<TInput> phaseShiftSource) {
    return pwmWave(inputSource, periodSource, phaseShiftSource, rheo::sources::constant((TFloat)0.5));
  }

}