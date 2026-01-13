#include <string_view>

template <uint8_t QtyType, typename Ref>
struct UnitDef {
    Ref magnitude;
    Ref offset;
    std::string_view* names;
};

template <uint8_t QtyType, typename Ref>
class DimensionedUnit {
    protected:
        int8_t power;

        UnitDef<QtyType, Ref>* definition;

    public:
        DimensionedUnit(UnitDef<QtyType, Ref>* def, int8_t power = 1)
            : power(power), definition(def) {}

        bool is_equivalent_to(DimensionedUnit<QtyType, Ref> const& other) const {
            return power == other.power;
        }

        bool get_power() const {
            return power;
        }

        Ref get_conversion_factor_for(DimensionedUnit<QtyType, Ref> const& other) const {
            if (definition == other.definition && power == other.power) {
                // No conversion necessary.
                return static_cast<Ref>(1);
            }

            if (power != other.power) {
                // Incompatible units.
                return static_cast<Ref>(0); // Placeholder for error handling
            }

            return static_cast<Ref>(definition.magnitude ^ power / other.definition.magnitude ^ power);
        }

        template <typename Power>
        DimensionedUnitPowerSafe<QtyType, Ref, Power> try_to_power_safe() const {
            if (power != Power) {
                // Incompatible power
                throw std::runtime_error("Incompatible power for dimensioned unit.");
            }
            return DimensionedUnitPowerSafe<QtyType, Ref, Power>(definition);
        }
};

template <uint8_t QtyType, typename Ref, int8_t Power>
class DimensionedUnitPowerSafe : DimensionedUnit<QtyType, Ref> {
    public:
        DimensionedUnitPowerSafe(UnitDef<QtyType, Ref>* def)
            : DimensionedUnit<QtyType, Ref>(def, power) {}

        DimensionedUnit<QtyType, Ref> to_unchecked() const {
            DimensionedUnit<QtyType, Ref> base_unit(this->definition, Power);
            return base_unit;
        }
};

template <typename... DimensionedUnits>
struct DefaultDimensions {
    std::tuple<DimensionedUnits...> quantity_types;
};

template <typename Dimensions, typename Ref>
struct Value {
    Ref value;
    Dimensions dimensions;
};
