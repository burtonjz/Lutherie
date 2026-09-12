/*
 * Copyright (C) 2026 Jared Burton
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

#ifndef __SCALE_TYPE_HPP_
#define __SCALE_TYPE_HPP_

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <array>
#include <cstdint> 
#include <type_traits>

using json = nlohmann::json ;

class ScaleType {
public:
    enum Value : uint8_t {
        MAJOR,
        NATURAL_MINOR,
        HARMONIC_MINOR,
        MELODIC_MINOR,
        PENTATONIC,
        PENTATONIC_MINOR,
        BLUES,
        DORIAN,
        PHRYGIAN,
        LYDIAN,
        MIXOLYDIAN,
        LOCRIAN,
        WHOLE_TONE,
        CHROMATIC,
        DIMINISHED,
        AUGMENTED,
        N_SCALE_TYPES
    };

    ScaleType() = default ;
    constexpr ScaleType(Value v) : value_(v){} 

    constexpr operator Value() const { return value_ ; }

    std::string toString() const {
        return std::string(names_[value_]);
    }

    static ScaleType fromString(std::string_view str) {
        for (int i = 0; i < N_SCALE_TYPES; ++i){
            if ( names_[i] == str){
                return static_cast<Value>(i);
            }
        }
        throw std::invalid_argument("unknown Scale Type: " + std::string(str));
    }

    uint8_t to_uint8() const {
        return value_ ;
    }

    static ScaleType from_uint8(uint8_t val){
        return ScaleType(static_cast<Value>(val));
    }

    static const std::array<std::string_view, N_SCALE_TYPES>& getNames(){
        return names_ ;
    }

    static constexpr int count = N_SCALE_TYPES ;
    static constexpr size_t MAX_INTERVALS = 12 ; 

    std::span<const uint8_t> intervals() const {
        return std::span<const uint8_t>(intervals_[value_].data(), counts_[value_]);
    }

private:
    Value value_ ;

    static constexpr std::array<std::string_view, N_SCALE_TYPES> names_{
        "Major",
        "Natural Minor",
        "Harmonic Minor",
        "Melodic Minor",
        "Pentatonic",
        "Pentatonic Minor",
        "Blues",
        "Dorian",
        "Phrygian",
        "Lydian",
        "Mixolydian",
        "Locrian",
        "Whole Tone",
        "Chromatic",
        "Diminished",
        "Augmented"
    };

    static constexpr std::array<std::array<uint8_t, MAX_INTERVALS>, N_SCALE_TYPES> intervals_{{
        {0,2,4,5,7,9,11}, // MAJOR
        {0,2,3,5,7,8,10}, // NATURAL_MINOR
        {0,2,3,5,7,8,11}, // HARMONIC_MINOR
        {0,2,3,5,7,8,9,10,11}, // MELODIC_MINOR
        {0,2,4,7,9}, // PENTATONIC
        {0,3,5,7,10}, // PENTATONIC_MINOR
        {0,3,5,6,7,10}, // BLUES
        {0,2,3,5,7,9,10}, // DORIAN
        {0,1,3,5,7,8,10}, // PHRYGIAN
        {0,2,4,6,7,9,11}, // LYDIAN
        {0,2,4,5,7,9,10}, // MIXOLYDIAN
        {0,2,4,5,6,8,10}, // LOCRIAN
        {0,2,4,6,8,10}, // WHOLE_TONE
        {0,1,2,3,4,5,6,7,8,9,10,11}, // CHROMATIC
        {0,2,3,5,6,8,9,11}, // DIMINISHED
        {0,3,4,7,8,11}, // AUGMENTED
    }};

    static constexpr std::array<size_t, N_SCALE_TYPES> counts_{
        7, // MAJOR
        7, // NATURAL_MINOR
        7, // HARMONIC_MINOR
        9, // MELODIC_MINOR
        5, // PENTATONIC
        5, // PENTATONIC_MINOR
        6, // BLUES
        7, // DORIAN
        7, // PHRYGIAN
        7, // LYDIAN
        7, // MIXOLYDIAN
        7, // LOCRIAN
        6, // WHOLE_TONE
        12, // CHROMATIC
        8, // DIMINISHED
        6 // AUGMENTED
    };

};

inline void from_json(const json& j, ScaleType& t){
    t = ScaleType::fromString(j.get_ref<const std::string&>());
}

inline void to_json(json& j, const ScaleType& t){
    j = t.toString();
}

#endif // __SCALE_TYPE_HPP_