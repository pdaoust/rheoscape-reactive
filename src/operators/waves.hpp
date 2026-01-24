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
  template <typename TInput, typename MapFn>
  auto wave(source_fn<TInput> input_source, source_fn<TInput> period_source, source_fn<TInput> phase_shift_source, MapFn&& wave_function)
  -> source_fn<return_of<std::decay_t<MapFn>>> {
    using TFloat = return_of<std::decay_t<MapFn>>;

    return map(
      combine(input_source, period_source, phase_shift_source),
      [wave_function = std::forward<MapFn>(wave_function)](std::tuple<TInput, TInput, TInput> value) {
        TInput input = std::get<0>(value);
        TInput period = std::get<1>(value);
        TInput phase_shift = std::get<2>(value);
        TInput input_adjusted = input + phase_shift;
        // Division of like units yields a dimensionless scalar.
        auto completed_cycles = floor(input_adjusted / period);
        // scalar * TInput gives TInput; TInput - TInput gives TInput.
        TInput remainder = input_adjusted - completed_cycles * period;
        TFloat pos_in_cycle = static_cast<TFloat>(remainder) / static_cast<TFloat>(period);
        return wave_function(pos_in_cycle);
      }
    );
  }

  template <typename TFloat = float, typename TInput>
  source_fn<TFloat> sine_wave(source_fn<TInput> input_source, source_fn<TInput> period_source, source_fn<TInput> phase_shift_source) {
    return wave(input_source, period_source, phase_shift_source, [](TFloat input) { return sin((TFloat)input * M_PI * 2); });
  }

  template <typename TFloat = float, typename TInput>
  source_fn<TFloat> sawtooth_wave(source_fn<TInput> input_source, source_fn<TInput> period_source, source_fn<TInput> phase_shift_source) {
    return wave(input_source, period_source, phase_shift_source, [](TFloat input) { return input * 2 - 1; });
  }

  template <typename TFloat = float, typename TInput>
  source_fn<TFloat> triangle_wave(source_fn<TInput> input_source, source_fn<TInput> period_source, source_fn<TInput> phase_shift_source) {
    return wave(
      input_source,
      period_source,
      phase_shift_source,
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
  source_fn<TFloat> pwm_wave(source_fn<TInput> input_source, source_fn<TInput> period_source, source_fn<TInput> phase_shift_source, source_fn<TFloat> duty_source) {
    return map(
      combine(
        wave(input_source, period_source, phase_shift_source, [](TFloat v) { return v; }),
        duty_source
      ),
      [](std::tuple<TFloat, TFloat> value) {
        return std::get<0>(value) < std::get<1>(value)
            ? (TFloat)1
            : (TFloat)-1;
      }
    );
  }

  template <typename TFloat = float, typename TInput>
  source_fn<TFloat> square_wave(source_fn<TInput> input_source, source_fn<TInput> period_source, source_fn<TInput> phase_shift_source) {
    return pwm_wave(input_source, period_source, phase_shift_source, rheo::sources::constant((TFloat)0.5));
  }

}