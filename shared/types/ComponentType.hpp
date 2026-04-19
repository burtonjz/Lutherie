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

#ifndef __COMPONENT_TYPE_HPP_
#define __COMPONENT_TYPE_HPP_

// X-Macro for components
#define COMPONENT_TYPE_LIST \
    X(Oscillator) \
    X(PolyOscillator) \
    X(BiquadFilter) \
    X(LinearFader) \
    X(ADSREnvelope) \
    X(MidiFilter) \
    X(Sequencer) \
    X(MonophonicFilter) \
    X(Delay) \
    X(Multiply) \
    X(SpectrumAnalyzer)

/**
 * @brief enumeration of modulation source classes
 * 
*/
enum class ComponentType {
    #define X(name) \
        name,
    COMPONENT_TYPE_LIST
    #undef X
    Unknown,
    N_COMPONENTS
};

constexpr int N_COMPONENT_TYPES = static_cast<int>(ComponentType::N_COMPONENTS) - 1 ;

// Define a converter from the enum to the actual type. 
// must be defined in synth/configs/
template <ComponentType Type> struct ComponentTypeTraits ;

// helper aliases
template<ComponentType Type>
using ComponentType_t = typename ComponentTypeTraits<Type>::type ;

template<ComponentType Type>
using ComponentConfig_t = typename ComponentTypeTraits<Type>::config ;

#endif // __COMPONENT_TYPE_HPP_