#pragma once

#include <functional>
#include <types/au_all_units_noio.hpp>
#include <types/core_types.hpp>
#include <util/misc.hpp>
#include <operators/lift.hpp>

namespace rheoscape::helpers {

  using TempC = au::QuantityPoint<au::Celsius, float>;
  using DegC = au::Quantity<au::Celsius, float>;
  using TempF = au::QuantityPoint<au::Fahrenheit, float>;
  using DegF = au::Quantity<au::Celsius, float>;
  using Percent = au::Quantity<au::Percent, float>;

  template <typename TRepOut, typename TRepIn, typename TUnit>
  map_fn<au::Quantity<TUnit, TRepOut>, au::Quantity<TUnit, TRepIn>> cast_unit_rep() {
    return au::rep_cast<TRepOut, TUnit, TRepIn>;
  }

  template <typename TRepOut, typename TRepIn, typename TUnit>
  map_fn<au::QuantityPoint<TUnit, TRepOut>, au::QuantityPoint<TUnit, TRepIn>> cast_unit_rep() {
    return au::rep_cast<TRepOut, TUnit, TRepIn>;
  }

  template <typename TUnitOut, typename TUnitIn, typename TRep>
  map_fn<au::Quantity<TUnitOut, TRep>, au::Quantity<TUnitIn, TRep>> convert_unit() {
    return [](au::Quantity<TUnitIn, TRep> value) { return value.as(TUnitOut{}); };
  }

  template <typename TUnitOut, typename TUnitIn, typename TRep>
  map_fn<au::QuantityPoint<TUnitOut, TRep>, au::QuantityPoint<TUnitIn, TRep>> convert_unit() {
    return [](au::QuantityPoint<TUnitIn, TRep> value) { return value.as(TUnitOut{}); };
  }

  // Conversion for things that can be converted to au quantities automatically
  template <typename TDuration>
  map_fn<decltype(au::as_quantity(TDuration{})), TDuration> convert_to_quantity() {
    return [](TDuration value) { return au::as_quantity(value); };
  }

  template <typename TUnit, typename TRep>
  map_fn<au::Quantity<TUnit, TRep>, TRep> convert_to_quantity() {
    return [](TRep value) { return au::Quantity<TUnit, TRep>(value); };
  }

  template <typename TUnit, typename TRep>
  map_fn<au::QuantityPoint<TUnit, TRep>, TRep> convert_to_quantity_point() {
    return [](TRep value) { return au::QuantityPoint<TUnit, TRep>(value); };
  }

  template <typename TUnit, typename TRep>
  map_fn<TRep, au::Quantity<TUnit, TRep>> strip_quantity() {
    return [](au::Quantity<TUnit, TRep> v) { return v.in(TUnit{}); };
  }

  template <typename TUnit, typename TRep>
  map_fn<TRep, au::QuantityPoint<TUnit, TRep>> strip_quantity_point() {
    return [](au::QuantityPoint<TUnit, TRep> v) { return v.in(TUnit{}); };
  }

  template <typename TUnit, typename TRep>
  pipe_fn<au::Quantity<TUnit, TRep>, au::Quantity<TUnit, TRep>> lift_to_quantity(pipe_fn<TRep, TRep> scalar_pipe) {
    return lift(
      scalar_pipe,
      convert_to_quantity<TUnit, TRep>(),
      strip_quantity<TUnit, TRep>()
    );
  }

  template <typename TUnit, typename TRep>
  pipe_fn<au::Quantity<TUnit, TRep>, au::Quantity<TUnit, TRep>> lift_to_quantity_point(pipe_fn<TRep, TRep> scalar_pipe) {
    return lift(
      scalar_pipe,
      convert_to_quantity_point<TUnit, TRep>(),
      strip_quantity_point<TUnit, TRep>()
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

template <typename TUnit, typename TRep>
struct fmt::formatter<au::Quantity<TUnit, TRep>> : fmt::formatter<std::string> {
  auto format(const au::Quantity<TUnit, TRep>& qty, format_context& ctx) const {
    return fmt::formatter<std::string>::format(au_to_string(qty), ctx);
  }
};

template <typename TUnit, typename TRep>
struct fmt::formatter<au::QuantityPoint<TUnit, TRep>> : fmt::formatter<std::string> {
  auto format(const au::QuantityPoint<TUnit, TRep>& qty, format_context& ctx) const {
    return fmt::formatter<std::string>::format(au_to_string(qty), ctx);
  }
};
