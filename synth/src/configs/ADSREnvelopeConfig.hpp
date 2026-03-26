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

#ifndef __HPP_CONFIGS_ADSRENVELOPE_
#define __HPP_CONFIGS_ADSRENVELOPE_

#include "types/ComponentType.hpp"
#include "types/MonophonicTriggerBehavior.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

// forward declare class
class ADSREnvelope ;

// define default configuration
struct ADSREnvelopeConfig {
    double attack = 0.1 ;
    double decay = 0.1 ;
    double sustain = 0.8 ;
    double release = 0.1 ; 
    MonophonicTriggerBehavior trigger = MonophonicTriggerBehavior::LEGATO ;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ADSREnvelopeConfig, attack, decay, sustain, release, trigger) // macro to serialize/deserialize json <-> structs

template <> struct ComponentTypeTraits<ComponentType::ADSREnvelope>{ 
    using type = ADSREnvelope ;
    using config = ADSREnvelopeConfig ;
};

#endif // __HPP_CONFIGS_ADSRENVELOPE_