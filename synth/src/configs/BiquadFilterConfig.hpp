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

#ifndef __HPP_CONFIGS_BIQUADFILTER_
#define __HPP_CONFIGS_BIQUADFILTER_

#include "types/ComponentType.hpp"
#include "types/FilterType.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

// forward declare class
class BiquadFilter ;

// define default configuration
struct BiquadFilterConfig {
    FilterType filterType = FilterType::LowPass ;
    double frequency = 1000 ;
    double gain = 1.0 ;
    double qFactor = 0.707 ;
    double bandwidth = 1.0 ;
    double shelfSlope = 2.0 ;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BiquadFilterConfig, filterType, frequency, gain, qFactor, bandwidth, shelfSlope) // macro to serialize/deserialize json <-> structs

template <> struct ComponentTypeTraits<ComponentType::BiquadFilter>{ 
    using type = BiquadFilter ;
    using config = BiquadFilterConfig ;
};

#endif // __HPP_CONFIGS_BIQUADFILTER_