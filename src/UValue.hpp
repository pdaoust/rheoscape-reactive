#ifndef UVALUE_HPP
#define UVALUE_HPP

template <typename TUnit, typename TVal>
struct UValue {
  TVal value;
  TUnit unit;

  UValue(TVal value, TUnit unit)
  :
    unit(unit),
    value(value)
  { }
};

enum TemperatureUnit {
  celsius,
  fahrenheit,
  kelvin
};

struct Temperature : public UValue<TemperatureUnit, float> {
  private:
    static const float cToK = 273.15f;
    static float cToF(float c) { return c * (9.0f / 5.0f) + 32; }
    static float fToC(float f) { return (f - 32) * (5.0f / 9.0f); }

    static float _convertTo(float value, TemperatureUnit fromUnit, TemperatureUnit toUnit) {
      if (fromUnit == toUnit) {
        return value;
      }

      switch (toUnit) {
        case TemperatureUnit::kelvin:
          switch (fromUnit) {
            case TemperatureUnit::celsius: return value + cToK;
            case TemperatureUnit::fahrenheit: return fToC(value) + cToK;
          }
        case TemperatureUnit::celsius:
          switch (fromUnit) {
            case TemperatureUnit::fahrenheit: return fToC(value);
            case TemperatureUnit::kelvin: return value - cToK;
          }
        case TemperatureUnit::fahrenheit:
          switch (fromUnit) {
            case TemperatureUnit::celsius: return cToF(value);
            case TemperatureUnit::kelvin: return cToF(value - cToK);
          }
      }
    }

  public:
    Temperature(float value, TemperatureUnit unit)
    : UValue<TemperatureUnit, float>(value, unit)
    { }

    static Temperature c(float value) {
      return Temperature(value, TemperatureUnit::celsius);
    }
    
    static Temperature f(float value) {
      return Temperature(value, TemperatureUnit::fahrenheit);
    }
    
    static Temperature k(float value) {
      return Temperature(value, TemperatureUnit::kelvin);
    }

    Temperature convertTo(TemperatureUnit toUnit) {
      if (toUnit == unit) {
        return *this;
      }
      return Temperature(_convertTo(value, unit, toUnit), toUnit);
    }
};

#endif