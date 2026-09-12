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

#ifndef __SCALE_NOTE_HPP_
#define __SCALE_NOTE_HPP_

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <cstdint> 
#include <type_traits>

using json = nlohmann::json ;

class ScaleNote {
public:
    enum Value : uint8_t {
        C = 0, 
        CSHARP_DFLAT = 1, 
        D = 2, 
        DSHARP_EFLAT = 3,
        E = 4, 
        F = 5, 
        FSHARP_GFLAT = 6, 
        G = 7,
        GSHARP_AFLAT = 8, 
        A = 9, 
        ASHARP_BFLAT = 10, 
        B = 11,
        N_SCALE_NOTES
    };

    ScaleNote() = default ;
    constexpr ScaleNote(Value v) : value_(v){} 

    constexpr operator Value() const { return value_ ; }

    std::string toString(bool preferSharp = true) const {
        if ( preferSharp ){
            return std::string(sharps_[value_]);
        } else {
            return std::string(flats_[value_]);
        }
    }

    static ScaleNote fromString(std::string_view str){
        for (int i = 0; i < N_SCALE_NOTES; ++i ){
            if ( sharps_[i] == str ){
                return static_cast<Value>(i);
            } 
            if ( flats_[i] == str ){
                return static_cast<Value>(i);
            }
        }
        throw std::invalid_argument("unknown Filter Type: " + std::string(str));
    }

    static const std::pair<ScaleNote, uint8_t> fromMidiValue(uint8_t midi){
        uint8_t octave = ( midi / 12 ) - 1 ;
        Value Value = static_cast<enum Value>(modulo(midi, 12));
        return {Value, octave};
    }

    static const std::array<std::string_view, N_SCALE_NOTES>& getNames(bool preferSharp = true){
        if ( preferSharp ) return sharps_ ;
        return flats_ ;
    }

    static constexpr int count = N_SCALE_NOTES ;

    static constexpr uint8_t getMidiValue(Value n, uint8_t octave, uint8_t interval = 0){
        return n + 12 * (octave + 1) + interval ;
    }

    static Value from_uint8(uint8_t val){
        return static_cast<Value>(static_cast<std::underlying_type_t<Value>>(val));
    }

    uint8_t to_uint8(){
        return static_cast<uint8_t>(value_) ;
    }

private:
    Value value_ ;

    static constexpr std::array<std::string_view, N_SCALE_NOTES> sharps_{
        "C",
        "C#",
        "D",
        "D#",
        "E",
        "F",
        "F#", 
        "G",
        "G#", 
        "A", 
        "A#", 
        "B"
    };

    static constexpr std::array<std::string_view, N_SCALE_NOTES> flats_{
        "C", 
        "Db",
        "D",
        "Eb",
        "E",
        "F",
        "Gb",
        "G",
        "Ab",
        "A",
        "Bb",
        "B"
    };

    static int modulo(int a, int b){
        return (a % b + b) % b ;
    }
};

inline void from_json(const json& j, ScaleNote& t){
    t = ScaleNote::fromString(j.get_ref<const std::string&>());
}

inline void to_json(json& j, const ScaleNote& t){
    j = t.toString();
}

#endif // __SCALE_TYPE_HPP_