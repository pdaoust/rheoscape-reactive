#pragma once

#include <math.h>
#include <functional>
#include <operators/map.hpp>
#include <operators/combine.hpp>
#include <sources/constant.hpp>

namespace rheoscape::operators {

  // Calculate the output of a wave function
  // given a source for the x axis (such as time)
  // and sources for the period length and phase shift.
  //
  // The input source type can differ from the period/phase type
  // as long as they are time point/duration compatible.
  // For instance, if the input type is a std::chrono time point source,
  // the period length and phase shift should be std::chrono duration sources.
  // For scalar types, all three can be the same type (int, float, etc).
  //
  // You need to pass in a mapper function that takes a floating-point value from 0 to 1
  // that represents how far through a period we are
  // and returns a Y-axis value of any type.
  // Here's an example for a sine wave:
  //
  // ```c++
  // // Turn a 0..1 value into radians.
  // // If you want double-precision math, just pass a mapper that accepts double instead.
  // [](float p) { return sin(p * M_PI * 2); }
  // ```

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT, typename MapFn>
    requires concepts::Source<InputSourceT>
      && concepts::Source<PeriodSourceT>
      && concepts::Source<PhaseSourceT>
      && concepts::Transformer<MapFn, TCalc>
      && std::is_same_v<source_value_t<PeriodSourceT>, source_value_t<PhaseSourceT>>
      && concepts::TimePointAndDurationCompatible<source_value_t<InputSourceT>, source_value_t<PeriodSourceT>>
  auto wave(InputSourceT input_source, PeriodSourceT period_source, MapFn&& wave_function, PhaseSourceT phase_shift_source = rheoscape::sources::constant(source_value_t<PeriodSourceT>{})) {
    using TInput = source_value_t<InputSourceT>;
    using TDuration = source_value_t<PeriodSourceT>;
    using MapFnDecayed = std::decay_t<MapFn>;

    struct WaveMapper {
      MapFnDecayed wave_function;

      RHEOSCAPE_CALLABLE auto operator()(std::tuple<TInput, TDuration, TDuration> value) const {
        auto [input, period, phase_shift] = value;
        // Convert input to duration-compatible space by subtracting epoch.
        // For scalars, TInput{} is 0, so this is identity.
        // For chrono time_points, this extracts time_since_epoch().
        auto elapsed = input - TInput{};
        // Adjust for phase shift, adding a full period to keep the value positive.
        auto adjusted = elapsed + (period - phase_shift);
        // floor() returns a floating-point type, which propagates through
        // the remaining arithmetic for proper fractional precision.
        auto completed_cycles = floor(adjusted / period);
        auto remainder = adjusted - completed_cycles * period;
        // Division of like types yields a dimensionless scalar.
        // For chrono: duration<double>/duration<int> gives double.
        // For scalars: double/int gives double.
        TCalc theta = static_cast<TCalc>(remainder / period);
        return wave_function(theta);
      }
    };

    return map(
      combine(std::move(input_source), std::move(period_source), std::move(phase_shift_source)),
      WaveMapper{ std::forward<MapFn>(wave_function) }
    );
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT>
      && concepts::Source<PeriodSourceT>
      && concepts::Source<PhaseSourceT>
      && std::is_same_v<source_value_t<PeriodSourceT>, source_value_t<PhaseSourceT>>
      && concepts::TimePointAndDurationCompatible<source_value_t<InputSourceT>, source_value_t<PeriodSourceT>>
  auto sine_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant(source_value_t<PeriodSourceT>{})) {
    struct SineFunction {
      RHEOSCAPE_CALLABLE TCalc operator()(TCalc theta) const {
        return sin(theta * M_PI * 2);
      }
    };

    return wave(std::move(input_source), std::move(period_source), SineFunction{}, std::move(phase_shift_source));
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT>
      && concepts::Source<PeriodSourceT>
      && concepts::Source<PhaseSourceT>
      && std::is_same_v<source_value_t<PeriodSourceT>, source_value_t<PhaseSourceT>>
      && concepts::TimePointAndDurationCompatible<source_value_t<InputSourceT>, source_value_t<PeriodSourceT>>
  auto sawtooth_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant(source_value_t<PeriodSourceT>{})) {
    struct SawtoothFunction {
      RHEOSCAPE_CALLABLE TCalc operator()(TCalc theta) const {
        return theta * 2 - 1;
      }
    };

    return wave(std::move(input_source), std::move(period_source), SawtoothFunction{}, std::move(phase_shift_source));
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT>
      && concepts::Source<PeriodSourceT>
      && concepts::Source<PhaseSourceT>
      && std::is_same_v<source_value_t<PeriodSourceT>, source_value_t<PhaseSourceT>>
      && concepts::TimePointAndDurationCompatible<source_value_t<InputSourceT>, source_value_t<PeriodSourceT>>
  auto triangle_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant(source_value_t<PeriodSourceT>{})) {
    struct TriangleFunction {
      RHEOSCAPE_CALLABLE TCalc operator()(TCalc theta) const {
        theta = theta * 4;
        if (theta < 1) {
          return theta;
        } else if (theta >= 1 && theta < 3) {
          return (theta - 2) * -1;
        } else {
          return theta - 4;
        }
      }
    };

    return wave(std::move(input_source), std::move(period_source), TriangleFunction{}, std::move(phase_shift_source));
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT>
    requires concepts::Source<InputSourceT>
      && concepts::Source<PeriodSourceT>
      && concepts::Source<PhaseSourceT>
      && std::is_same_v<source_value_t<PeriodSourceT>, source_value_t<PhaseSourceT>>
      && concepts::TimePointAndDurationCompatible<source_value_t<InputSourceT>, source_value_t<PeriodSourceT>>
  auto square_wave(InputSourceT input_source, PeriodSourceT period_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant(source_value_t<PeriodSourceT>{})) {
    struct SquareFunction {
      RHEOSCAPE_CALLABLE TCalc operator()(TCalc theta) const {
        return theta < 0.5
          ? 1.0
          : -1.0;
      }
    };

    return wave(std::move(input_source), std::move(period_source), SquareFunction{}, std::move(phase_shift_source));
  }

  template <typename TCalc = float, typename InputSourceT, typename PeriodSourceT, typename PhaseSourceT, typename DutySourceT>
    requires concepts::Source<InputSourceT>
      && concepts::Source<PeriodSourceT>
      && concepts::Source<PhaseSourceT>
      && concepts::Source<DutySourceT>
      && std::is_same_v<source_value_t<PeriodSourceT>, source_value_t<PhaseSourceT>>
      && concepts::TimePointAndDurationCompatible<source_value_t<InputSourceT>, source_value_t<PeriodSourceT>>
      && std::is_same_v<TCalc, source_value_t<DutySourceT>>
  auto pwm_wave(InputSourceT input_source, PeriodSourceT period_source, DutySourceT duty_source, PhaseSourceT phase_shift_source = rheoscape::sources::constant(source_value_t<PeriodSourceT>{})) {
    struct IdentityFunction {
      RHEOSCAPE_CALLABLE TCalc operator()(TCalc theta) const {
        return theta;
      }
    };

    struct DutyComparator {
      RHEOSCAPE_CALLABLE bool operator()(std::tuple<TCalc, TCalc> theta) const {
        // While a sine wave goes from -1 to 1, that's never useful for PWM and square waves.
        // Instead, go from 0 to 1.
        return std::get<0>(theta) < std::get<1>(theta)
            ? true
            : false;
      }
    };

    return map(
      combine(
        wave(std::move(input_source), std::move(period_source), IdentityFunction{}, std::move(phase_shift_source)),
        std::move(duty_source)
      ),
      DutyComparator{}
    );
  }

}
