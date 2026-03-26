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

class MonophonicTriggerBehavior {
public:
    enum Behavior : uint8_t {
        LEGATO,
        RETRIGGER_LEGATO,
        RETRIGGER_RESET,
        N
    };

    MonophonicTriggerBehavior() = default ;
    // construct from enum
    constexpr MonophonicTriggerBehavior(Behavior b) : behavior_(b){} 

    // construct from string
    MonophonicTriggerBehavior(std::string_view name){
        if      ( name == "Legato" )             behavior_ = LEGATO  ;
        else if ( name == "Retrigger (Legato)" ) behavior_ = RETRIGGER_LEGATO ;
        else if ( name == "Retrigger (Reset)" )  behavior_ = RETRIGGER_RESET ;
        else throw std::invalid_argument("Unknown monophonic trigger behavior: " + std::string(name));
    }

    // allow switch / comparisons
    constexpr operator Behavior() const { return behavior_ ; }

    // prevent bool usage e.g., if(MonophonicTriggerBehavior)
    explicit operator bool() const = delete ;

    static std::string toString(MonophonicTriggerBehavior w){
        switch(w){
        case LEGATO:              return "Legato" ;
        case RETRIGGER_LEGATO:    return "Retrigger (Legato)" ;
        case RETRIGGER_RESET:     return "Retrigger (Reset)" ;
        default:                  return "" ;
        };
    }

    std::string toString() const {
        return MonophonicTriggerBehavior::toString(behavior_);
    }

    static std::array<std::string_view, N> getBehaviors(){
        return {
            "Legato",
            "Retrigger (Legato)",
            "Retrigger (Reset)",
        } ;
    }

    static Behavior from_uint8(uint8_t val){
        return static_cast<Behavior>(static_cast<std::underlying_type_t<Behavior>>(val));
    }

    uint8_t to_uint8(){
        return static_cast<uint8_t>(behavior_) ;
    }

private:
    Behavior behavior_ ;
};

inline void to_json(json& j, const MonophonicTriggerBehavior& w){
        j = w.toString();
    }

inline void from_json(const json& j, MonophonicTriggerBehavior& w){
    w = MonophonicTriggerBehavior(j.get<std::string>());
}

#endif // __MONOPHONIC_TRIGGER_BEHAVIOR_HPP_
