/*
 * Copyright (C) 2025 Jared Burton
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "types/ParameterType.hpp"
#include "types/ModulationStrategy.hpp"
#include "types/ParameterTraits.hpp"

uint8_t ParameterType::to_uint8(ParameterType p){
    return static_cast<uint8_t>(p.value_);
}

uint8_t ParameterType::to_uint8() const {
    return static_cast<uint8_t>(value_);
}

ParameterType ParameterType::from_uint8(uint8_t val){
    return ParameterType(static_cast<Value>(val));
}

std::string ParameterType::toString(ParameterType p){
    return visit(p, [](auto traits){
        return std::string(decltype(traits)::name);
    });
}

std::string ParameterType::toString() const {
    return toString(value_);
}

ParameterType ParameterType::fromString(std::string_view str){
    std::optional<ParameterType> result ;

    #define X(NAME) \
        if ( str == ParameterTraits<Value::NAME>::name ) result = Value::NAME ;
        PARAMETER_TYPE_LIST
    #undef X

    if ( result ) return *result ;
    
    throw std::invalid_argument("Unknown ParameterType: " + std::string(str));
}

ParameterSupports ParameterType::supports(ParameterType p){
    return visit(p, [](auto traits){
        using T = decltype(traits);

        return ParameterSupports{
            T::Supports::rangeUpdate,
            T::Supports::modulatable,
            T::Supports::multiValue
        };
    });
}

ParameterSupports ParameterType::supports() const {
    return supports(value_);
}

ParameterLimits ParameterType::limits(ParameterType p){
    return visit(p, [](auto traits){
        using T = decltype(traits);

        return ParameterLimits{
            T::Limits::minimum,
            T::Limits::maximum
        };
    });
}

ParameterLimits ParameterType::limits() const {
    return limits(value_);
}

ParameterValue ParameterType::defaultValue(ParameterType p){
    return visit(p, [](auto traits) -> ParameterValue {
        using T = decltype(traits);
        return T::Defaults::value ;
    });
}

ParameterValue ParameterType::defaultValue() const {
    return defaultValue(value_);
}

ModulationStrategy ParameterType::defaultModulationStrategy(ParameterType p, ModulatorRange range){
    return visit(p, [range](auto traits) -> ModulationStrategy {
        using T = decltype(traits);

        for ( const auto& [r, s]: T::Defaults::modulationStrategy){
            if ( range == r ) return s ;
        }
        return ModulationStrategy::NONE ;
    });
}

ModulationStrategy ParameterType::defaultModulationStrategy(ModulatorRange range) const {
    return defaultModulationStrategy(value_, range);
}

// JSON
inline void from_json(const json& j, ParameterType& t){
    t = ParameterType::fromString(j.get_ref<const std::string&>());
}

inline void to_json(json& j, const ParameterType& t){
    j = t.toString();
};

inline void from_json(const json& j, ParameterSupports& supports){
    supports.modulatable = j["modulatable"];
    supports.multiValue = j["multiValue"];
    supports.rangeUpdate = j["rangeUpdate"];
}

inline void to_json(json& j, const ParameterSupports& supports){
    j["modulatable"] = supports.modulatable ;
    j["multiValue"] = supports.multiValue ;
    j["rangeUpdate"] = supports.rangeUpdate ;
}

inline void from_json(const json& j, ParameterLimits& limits){
    limits.maximum = j["maximum"].get<ParameterValue>();
    limits.minimum = j["minimum"].get<ParameterValue>();
}

inline void to_json(json& j, const ParameterLimits& limits){
    j["maximum"] = limits.maximum ;
    j["minimum"] = limits.minimum ;
}