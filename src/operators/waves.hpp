#pragma once

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
  template <typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT, typename MapFn>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source, MapFn&& wave_function) {
    using TInput = source_value_t<InputSourceT>;
    using TFloat = return_of<std::decay_t<MapFn>>;
    using MapFnDecayed = std::decay_t<MapFn>;

    struct WaveMapper {
      MapFnDecayed wave_function;

      RHEO_CALLABLE TFloat operator()(std::tuple<TInput, TInput, TInput> value) const {
        TInput input = std::get<0>(value);
        TInput period = std::get<1>(value);
        TInput phase_shift = std::get<2>(value);
        TInput input_adjusted = input + phase_shift;
        // Division of like units yields a dimensionless scalar.
        auto completed_cycles = floor(input_adjusted / period);
        // scalar * TInput gives TInput; TInput - TInput gives TInput.
        TInput remainder = input_adjusted - completed_cycles * period;
        // For std::chrono::duration, division of like durations yields a dimensionless scalar.
        // For scalar types, convert to TFloat first to avoid integer division.
        TFloat pos_in_cycle;
        if constexpr (std::is_arithmetic_v<TInput>) {
          pos_in_cycle = static_cast<TFloat>(remainder) / static_cast<TFloat>(period);
        } else {
          pos_in_cycle = static_cast<TFloat>(remainder / period);
        }
        return wave_function(pos_in_cycle);
      }
    };

    return map(
      combine(std::move(input_source), std::move(period_source), std::move(phase_shift_source)),
      WaveMapper{ std::forward<MapFn>(wave_function) }
    );
  }

  template <typename TFloat = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto sine_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source) {
    struct SineFunction {
      RHEO_CALLABLE TFloat operator()(TFloat input) const {
        return sin((TFloat)input * M_PI * 2);
      }
    };

    return wave(std::move(input_source), std::move(period_source), std::move(phase_shift_source), SineFunction{});
  }

  template <typename TFloat = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto sawtooth_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source) {
    struct SawtoothFunction {
      RHEO_CALLABLE TFloat operator()(TFloat input) const {
        return input * 2 - 1;
      }
    };

    return wave(std::move(input_source), std::move(period_source), std::move(phase_shift_source), SawtoothFunction{});
  }

  template <typename TFloat = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto triangle_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source) {
    struct TriangleFunction {
      RHEO_CALLABLE TFloat operator()(TFloat input) const {
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

  template <typename TFloat = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT, typename DutySourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> &&
             concepts::Source<PhaseSourceT> && concepts::Source<DutySourceT>
  auto pwm_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source, DutySourceT duty_source) {
    struct IdentityFunction {
      RHEO_CALLABLE TFloat operator()(TFloat v) const {
        return v;
      }
    };

    struct DutyComparator {
      RHEO_CALLABLE TFloat operator()(std::tuple<TFloat, TFloat> value) const {
        return std::get<0>(value) < std::get<1>(value)
            ? (TFloat)1
            : (TFloat)-1;
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

  template <typename TFloat = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT> && concepts::Source<PeriodSourceT> && concepts::Source<PhaseSourceT>
  auto square_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source) {
    return pwm_wave(std::move(input_source), std::move(period_source), std::move(phase_shift_source), rheo::sources::constant((TFloat)0.5));
  }

}
