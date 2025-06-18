#pragma once

#include <functional>
#include <types/au_all_units_noio.hpp>
#include <core_types.hpp>
#include <util.hpp>
#include <operators/lift.hpp>

namespace rheo {

  using TempC = au::QuantityPoint<au::Celsius, float>;
  using DegC = au::Quantity<au::Celsius, float>;
  using TempF = au::QuantityPoint<au::Fahrenheit, float>;
  using DegF = au::Quantity<au::Celsius, float>;
  using Percent = au::Quantity<au::Percent, float>;

  template <typename TRepOut, typename TRepIn, typename TUnit>
  map_fn<au::Quantity<TUnit, TRepOut>, au::Quantity<TUnit, TRepIn>> castUnitRep() {
    return au::rep_cast<TRepOut, TUnit, TRepIn>;
  }

  template <typename TRepOut, typename TRepIn, typename TUnit>
  map_fn<au::QuantityPoint<TUnit, TRepOut>, au::QuantityPoint<TUnit, TRepIn>> castUnitRep() {
    return au::rep_cast<TRepOut, TUnit, TRepIn>;
  }

  template <typename TUnitOut, typename TUnitIn, typename TRep>
  map_fn<au::Quantity<TUnitOut, TRep>, au::Quantity<TUnitIn, TRep>> convertUnit() {
    return [](au::Quantity<TUnitIn, TRep> value) { return value.as(TUnitOut{}); };
  }

  template <typename TUnitOut, typename TUnitIn, typename TRep>
  map_fn<au::QuantityPoint<TUnitOut, TRep>, au::QuantityPoint<TUnitIn, TRep>> convertUnit() {
    return [](au::QuantityPoint<TUnitIn, TRep> value) { return value.as(TUnitOut{}); };
  }

  // Conversion for things that can be converted to au quantities automatically
  template <typename TDuration>
  map_fn<decltype(au::as_quantity(TDuration{})), TDuration> convertToQuantity() {
    return [](TDuration value) { return au::as_quantity(value); };
  }

  template <typename TUnit, typename TRep>
  map_fn<au::Quantity<TUnit, TRep>, TRep> convertToQuantity() {
    return [](TRep value) { return au::Quantity<TUnit, TRep>(value); };
  }

  template <typename TUnit, typename TRep>
  map_fn<au::QuantityPoint<TUnit, TRep>, TRep> convertToQuantityPoint() {
    return [](TRep value) { return au::QuantityPoint<TUnit, TRep>(value); };
  }

  template <typename TUnit, typename TRep>
  map_fn<TRep, au::Quantity<TUnit, TRep>> stripQuantity() {
    return [](au::Quantity<TUnit, TRep> v) { return v.in(TUnit{}); };
  }

  template <typename TUnit, typename TRep>
  map_fn<TRep, au::QuantityPoint<TUnit, TRep>> stripQuantityPoint() {
    return [](au::QuantityPoint<TUnit, TRep> v) { return v.in(TUnit{}); };
  }

  template <typename TUnit, typename TRep>
  pipe_fn<au::Quantity<TUnit, TRep>, au::Quantity<TUnit, TRep>> liftToQuantity(pipe_fn<TRep, TRep> scalarPipe) {
    return lift(
      scalarPipe,
      convertToQuantity<TUnit, TRep>(),
      stripQuantity<TUnit, TRep>()
    );
  }

  template <typename TUnit, typename TRep>
  pipe_fn<au::Quantity<TUnit, TRep>, au::Quantity<TUnit, TRep>> liftToQuantityPoint(pipe_fn<TRep, TRep> scalarPipe) {
    return lift(
      scalarPipe,
      convertToQuantityPoint<TUnit, TRep>(),
      stripQuantityPoint<TUnit, TRep>()
    );
  }

  template <typename TUnit, typename TRep>
  std::string au_to_string(au::Quantity<TUnit, TRep> qty, uint8_t precision = 0) {
    return string_format("%.*f%s", precision, qty.in(TUnit{}), unit_label(TUnit{}));
  }

  template <typename TUnit, typename TRep>
  std::string au_to_string(au::QuantityPoint<TUnit, TRep> qty, uint8_t precision = 0) {
    return string_format("%.*f%s", precision, qty.in(TUnit{}), unit_label(TUnit{}));
  }

}