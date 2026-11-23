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

#ifndef __PARAMETER_TYPE_HPP_
#define __PARAMETER_TYPE_HPP_

#include "types/ParameterTraits.hpp"
#include <stdexcept>

struct ParameterSupports {
    bool rangeUpdate ;
    bool modulatable ;
    size_t maxSlots ;

    bool operator==(const ParameterSupports& other) const {
        return rangeUpdate == other.rangeUpdate
            && modulatable == other.modulatable
            && maxSlots == other.maxSlots ;
    }
};

inline void from_json(const json& j, ParameterSupports& supports);
inline void to_json(json& j, const ParameterSupports& supports);

struct ParameterLimits {
    ParameterValue minimum ;
    ParameterValue maximum ;

    bool operator==(const ParameterLimits& other) const {
        return minimum == other.minimum
            && maximum == other.maximum ;
    } 
};

inline void from_json(const json& j, ParameterLimits& limits);
inline void to_json(json& j, const ParameterLimits& limits);

class ParameterType {
public:
    using Value = _parameter_detail::Type ;

    #define X(NAME) static constexpr auto NAME = _parameter_detail::Type::NAME ;
    PARAMETER_TYPE_LIST
    #undef X

    ParameterType() = default ;
    constexpr ParameterType(Value v): value_(v){} 
    constexpr operator Value() const { return value_ ; }
    static constexpr int count = static_cast<size_t>(Value::N_PARAMETER_TYPES);

    template <typename Func>
    static decltype(auto) visit(ParameterType p, Func&& func){
        switch (p){
            #define X(NAME) \
                case NAME: \
                    return std::forward<Func>(func)(ParameterTraits<NAME>{});
            PARAMETER_TYPE_LIST
            #undef X
            default:
                throw std::runtime_error("unknown ParameterType");
        }
    }

    uint8_t to_uint8() const ;
    static uint8_t to_uint8(ParameterType p);
    static ParameterType from_uint8(uint8_t val);

    std::string toString() const ;
    static std::string toString(ParameterType p);
    static ParameterType fromString(std::string_view str);

    static json valueToJson(ParameterType p, const ParameterValue& v);

    ParameterSupports supports() const ;
    static ParameterSupports supports(ParameterType p);

    ParameterLimits limits() const ;
    static ParameterLimits limits(ParameterType p);

    ParameterValue defaultValue() const ;
    static ParameterValue defaultValue(ParameterType p);

    ModulationStrategy defaultModulationStrategy(ModulatorRange range) const ;
    static ModulationStrategy defaultModulationStrategy(ParameterType p, ModulatorRange range);

private:
    Value value_ ;

};

inline void from_json(const json& j, ParameterType& t);
inline void to_json(json& j, const ParameterType& t);

template <ParameterType::Value Type>
using ParameterValueType = typename ParameterTraits<Type>::ValueType ;

#endif // __PARAMETER_TYPE_HPP_