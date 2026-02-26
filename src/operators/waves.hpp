#pragma once

#include <math.h>
#include <functional>
#include <operators/map.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>

namespace rheoscape::operators {

  // Calculate the ouptut of a wave function
  // given a source for the t axis (such as time)
  // and sources for the period length and phase shift.
  // Note that the phase shift is probably backwards to what you're expecting --
  // if the period is 100 units and the phase shift is 25 units,
  // this doesn't mean the wave will be shifted forward along the t axis by 25 units.
  // All you need to do, really, is reverse your thinking.
  // For example, 25 becomes 100 - 25 = 75.
  template <typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT, typename MapFn>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant((TCalc)0.0), MapFn&& wave_function) {
    using TInput = source_value_t<InputSourceT>;
    using TCalc = return_of<std::decay_t<MapFn>>;
    using MapFnDecayed = std::decay_t<MapFn>;

    struct WaveMapper {
      MapFnDecayed wave_function;

      RHEOSCAPE_CALLABLE TCalc operator()(std::tuple<TInput, TInput, TInput> value) const {
        TInput input = std::get<0>(value);
        TInput period = std::get<1>(value);
        TInput phase_shift = std::get<2>(value);
        TInput input_adjusted = input + phase_shift;
        // Division of like units yields a dimensionless scalar.
        auto completed_cycles = floor(input_adjusted / period);
        // scalar * TInput gives TInput; TInput - TInput gives TInput.
        TInput remainder = input_adjusted - completed_cycles * period;
        // For std::chrono::duration, division of like durations yields a dimensionless scalar.
        // For scalar types, convert to TCalc first to avoid integer division.
        TCalc pos_in_cycle;
        if constexpr (std::is_arithmetic_v<TInput>) {
          pos_in_cycle = static_cast<TCalc>(remainder) / static_cast<TCalc>(period);
        } else {
          pos_in_cycle = static_cast<TCalc>(remainder / period);
        }
        return wave_function(pos_in_cycle);
      }
    };

    return map(
      combine(std::move(input_source), std::move(period_source), std::move(phase_shift_source)),
      WaveMapper{ std::forward<MapFn>(wave_function) }
    );
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto sine_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant((TCalc)0.0)) {
    struct SineFunction {
      RHEOSCAPE_CALLABLE TCalc operator()(TCalc input) const {
        return sin((TCalc)input * M_PI * 2);
      }
    };

    return wave(std::move(input_source), std::move(period_source), std::move(phase_shift_source), SineFunction{});
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto sawtooth_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant((TCalc)0.0)) {
    struct SawtoothFunction {
      RHEOSCAPE_CALLABLE TCalc operator()(TCalc input) const {
        return input * 2 - 1;
      }
    };

    return wave(std::move(input_source), std::move(period_source), std::move(phase_shift_source), SawtoothFunction{});
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto triangle_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant((TCalc)0.0)) {
    struct TriangleFunction {
      RHEOSCAPE_CALLABLE TCalc operator()(TCalc input) const {
        input = input * 4;
        if (input < 1) {
          return input;
        } else if (input >= 1 && input < 3) {
          return (input - 2) * -1;
        } else {
          return input - 4;
        }
      }
    };

    return wave(std::move(input_source), std::move(period_source), std::move(phase_shift_source), TriangleFunction{});
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT, typename DutySourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> &&
             concepts::Source<PhaseSourceT> && concepts::Source<DutySourceT>
  auto pwm_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant((TCalc)0.0), DutySourceT duty_source) {
    struct IdentityFunction {
      RHEOSCAPE_CALLABLE TCalc operator()(TCalc v) const {
        return v;
      }
    };

    struct DutyComparator {
      RHEOSCAPE_CALLABLE bool operator()(std::tuple<TCalc, TCalc> value) const {
        // While a sine wave goes from -1 to 1, that's never useful for PWM and square waves.
        // Instead, go from 0 to 1.
        return std::get<0>(value) < std::get<1>(value)
            ? true
            : false;
      }
    };

    return map(
      combine(
        wave(std::move(input_source), std::move(period_source), std::move(phase_shift_source), IdentityFunction{}),
        std::move(duty_source)
      ),
      DutyComparator{}
    );
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto square_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant((TCalc)0.0)) {
    return pwm_wave(std::move(input_source), std::move(period_source), std::move(phase_shift_source), rheoscape::sources::constant((TCalc)0.5));
  }

}
