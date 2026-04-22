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

#ifndef __HPP_ALL_CONFIGS_
#define __HPP_ALL_CONFIGS_

#include "types/ComponentType.hpp"

#include "configs/OscillatorConfig.hpp"
#include "configs/PolyOscillatorConfig.hpp"
#include "configs/BiquadFilterConfig.hpp"
#include "configs/LinearFaderConfig.hpp"
#include "configs/ADSREnvelopeConfig.hpp"
#include "configs/MidiFilterConfig.hpp"
#include "configs/SequencerConfig.hpp"
#include "configs/MonophonicFilterConfig.hpp"
#include "configs/DelayConfig.hpp"
#include "configs/MultiplyConfig.hpp"
#include "configs/SpectrumAnalyzerConfig.hpp"
#include "configs/OscilloscopeConfig.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json ;

inline json getDefaultConfig(ComponentType type){
    switch(type){
        #define X(NAME) \
            case ComponentType::NAME: return ComponentTypeTraits<ComponentType::NAME>::config{};
        COMPONENT_TYPE_LIST
        #undef X
    default:
        return json::object();
    }
}

#endif // __HPP_ALL_CONFIGS_