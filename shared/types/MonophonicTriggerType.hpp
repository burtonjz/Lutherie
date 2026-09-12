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

#ifndef __MONOPHONIC_TRIGGER_BEHAVIOR_HPP_
#define __MONOPHONIC_TRIGGER_BEHAVIOR_HPP_

#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class MonophonicTriggerType {
public:
    enum Value : uint8_t {
        LEGATO,
        RETRIGGER_LEGATO,
        RETRIGGER_RESET,
        N_MONOHPONIC_TRIGGER_TYPES
    };

    MonophonicTriggerType() = default ;
    constexpr MonophonicTriggerType(Value v) : value_(v){} 

    constexpr operator Value() const { return value_ ; }

    std::string toString() const {
        return std::string(names_[value_]);
    }

    static MonophonicTriggerType fromString(std::string_view str){
        for (int i = 0; i < N_MONOHPONIC_TRIGGER_TYPES; ++i ){
            if ( names_[i] == str ){
                return static_cast<Value>(i);
            }
        }
        throw std::invalid_argument("unknown Filter Type: " + std::string(str));
    }

    uint8_t to_uint8() const {
        return value_ ;
    }

    static MonophonicTriggerType from_uint8(uint8_t val){
        return MonophonicTriggerType(static_cast<Value>(val));
    }

    static const std::array<std::string_view, N_MONOHPONIC_TRIGGER_TYPES>& getNames(){
        return names_ ;
    }

    static constexpr int count = N_MONOHPONIC_TRIGGER_TYPES ;

private:
    Value value_ ;

    static constexpr std::array<std::string_view, N_MONOHPONIC_TRIGGER_TYPES> names_{
        "Legato",
        "Retrigger (Legato)",
        "Retrigger (Reset)"
    };
};

inline void from_json(const json& j, MonophonicTriggerType& t){
    t = MonophonicTriggerType::fromString(j.get_ref<const std::string&>());
}

inline void to_json(json& j, const MonophonicTriggerType& t){
    j = t.toString();
}

#endif // __MONOPHONIC_TRIGGER_BEHAVIOR_HPP_
