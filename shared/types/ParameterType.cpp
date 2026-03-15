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

json ParameterValueToJson(const ParameterValue& v){
    return std::visit([](auto&& v) -> json {
        if constexpr ( std::is_same_v<std::decay_t<decltype(v)>, uint8_t> ){
            return static_cast<int>(v);
        } else return v ;
    }, v);
}

ParameterType parameterFromString(std::string str) {
    static const std::unordered_map<std::string, ParameterType> str2Type = {
        #define X(NAME) {ParameterTraits<ParameterType::NAME>::name, ParameterType::NAME},
        PARAMETER_TYPE_LIST
        #undef X
    };
    
    auto it = str2Type.find(str);
    if ( it != str2Type.end() ){
        return it->second;
    }
    throw std::runtime_error("Unknown parameter name: " + str);
}

ModulationStrategy modStrategyFromString(std::string str){
    static const std::unordered_map<std::string, ModulationStrategy> str2strat = {
        #define X(NAME) {#NAME, ModulationStrategy::NAME},
        MODULATION_STRATEGY_LIST
        #undef X
    };

    auto it = str2strat.find(str);
    if ( it != str2strat.end() ){
        return it->second;
    }
    throw std::invalid_argument("Unknown ModulationStrategy: " + str);
}

std::string modStrategyToString(ModulationStrategy str){
    static const std::unordered_map<ModulationStrategy, std::string> strat2str = {
        #define X(NAME) {ModulationStrategy::NAME, #NAME},
        MODULATION_STRATEGY_LIST
        #undef X
    };

    auto it = strat2str.find(str);
    if ( it != strat2str.end() ){
        return it->second;
    }
        
    throw std::invalid_argument("Unknown ModulationStrategy." );
}