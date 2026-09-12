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

#ifndef __WAVEFORM_HPP_
#define __WAVEFORM_HPP_

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <array>
#include <cstdint> 
#include <type_traits>

using json = nlohmann::json ;

class Waveform {
public:
    enum Value : uint8_t {
        SINE = 0, 
        SQUARE,
        TRIANGLE,
        SAW,
        NOISE,
        N_WAVEFORMS
    };

    Waveform() = default ;
    constexpr Waveform(Value v) : value_(v){} 

    constexpr operator Value() const { return value_ ; }

    std::string toString() const {
        return std::string(names_[value_]);
    }

    static Waveform fromString(std::string_view str){
        for (int i = 0; i < N_WAVEFORMS; ++i){
            if ( names_[i] == str){
                return static_cast<Value>(i);
            }
        }
        throw std::invalid_argument("unknown Waveform: " + std::string(str));
    }

    uint8_t to_uint8() const {
        return value_ ;
    }

    static Waveform from_uint8(uint8_t val){
        return Waveform(static_cast<Value>(val));
    }

    static const std::array<std::string_view, N_WAVEFORMS>& getNames(){
        return names_ ;
    }

    static constexpr int count = N_WAVEFORMS ;

private:
    Value value_{SINE} ;

    static constexpr std::array<std::string_view, N_WAVEFORMS> names_{
        "SINE",
        "SQUARE",
        "TRIANGLE",
        "SAW",
        "NOISE"
    };
};

inline void from_json(const json& j, Waveform& t){
    t = Waveform::fromString(j.get_ref<const std::string&>());
}

inline void to_json(json& j, const Waveform& t){
    j = t.toString();
}


#endif // __WAVEFORM_HPP_