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

#ifndef MODULATION_STRATEGY_HPP_
#define MODULATION_STRATEGY_HPP_

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class ModulationStrategy {
public:
    enum Value : uint8_t {
        NONE,
        ADDITIVE,
        MULTIPLICATIVE_ZERO,
        MULTIPLICATIVE_UNITY,
        EXPONENTIAL,
        REPLACE,
        N_STRATEGIES
    };

    ModulationStrategy() = default ;
    constexpr ModulationStrategy(Value v) : value_(v){} 

    constexpr operator Value() const { return value_ ; }

    std::string toString() const {
        return std::string(names_[value_]);
    }

    static ModulationStrategy fromString(std::string_view str) {
        for (int i = 0; i < N_STRATEGIES; ++i){
            if ( names_[i] == str){
                return static_cast<Value>(i);
            }
        }
        throw std::invalid_argument("unknown Scale Type: " + std::string(str));
    }

    uint8_t to_uint8() const {
        return value_ ;
    }

    static ModulationStrategy from_uint8(uint8_t val){
        return ModulationStrategy(static_cast<Value>(val));
    }

    static const std::array<std::string_view, N_STRATEGIES>& getNames(){
        return names_ ;
    }

    static constexpr int count = N_STRATEGIES ;

private:
    Value value_ ;

    static constexpr std::array<std::string_view, N_STRATEGIES> names_{
        "None",
        "Additive",
        "Multiplicative (zero)",
        "Multiplicative (unity)",
        "Exponential",
        "Replace"
    };
};

inline void from_json(const json& j, ModulationStrategy& t){
    t = ModulationStrategy::fromString(j.get_ref<const std::string&>());
}

inline void to_json(json& j, const ModulationStrategy& t){
    j = t.toString();
}

enum class ModulatorRange : uint8_t {
    UNIPOLAR,
    BIPOLAR,
    UNKNOWN
};

#endif // MODULATION_STRATEGY_HPP_