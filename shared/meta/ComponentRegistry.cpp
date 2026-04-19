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

#include "meta/ComponentRegistry.hpp"
#include "meta/CollectionDescriptor.hpp"
#include "meta/ComponentDescriptor.hpp"
#include "types/ComponentType.hpp"
#include "types/CollectionType.hpp"
#include "types/ParameterType.hpp"
#include <stdexcept>

const std::unordered_map<ComponentType, ComponentDescriptor>& ComponentRegistry::getAllComponentDescriptors(){
    static const std::unordered_map<ComponentType, ComponentDescriptor> registry = {
        {
            ComponentType::Oscillator,
            {
                "Oscillator",
                ComponentType::Oscillator,
                {ParameterType::AMPLITUDE, ParameterType::FREQUENCY}, // modulatable params
                {ParameterType::WAVEFORM, ParameterType::AMPLITUDE, ParameterType::FREQUENCY}, //control params
                {},
                0,
                1,
                0, 
                0,
                true
            }
        },
        {
            ComponentType::PolyOscillator, 
            {
                "Polyphonic Oscillator",
                ComponentType::PolyOscillator,
                {ParameterType::AMPLITUDE, ParameterType::FREQUENCY, ParameterType::PHASE}, // modulatable params
                {ParameterType::DETUNE, ParameterType::WAVEFORM}, //control params
                {},
                0, 
                1, 
                1,
                0, 
                false
            }
        },
        {
            ComponentType::LinearFader,
            {
                "Linear Fader",
                ComponentType::LinearFader,
                {ParameterType::ATTACK, ParameterType::RELEASE},
                {ParameterType::ATTACK, ParameterType::RELEASE},
                {},
                0,
                0,
                1,
                1,
                true
            }
        },
        {
            ComponentType::ADSREnvelope,
            {
                "ADSR Envelope",
                ComponentType::ADSREnvelope,
                {ParameterType::ATTACK, ParameterType::DECAY, ParameterType::SUSTAIN, ParameterType::RELEASE},
                {ParameterType::ATTACK, ParameterType::DECAY, ParameterType::SUSTAIN, ParameterType::RELEASE, ParameterType::TRIGGER},
                {},
                0,
                0,
                1,
                1,
                true
            }
        },
        {
            ComponentType::MidiFilter,
            {
                "Midi Filter",
                ComponentType::MidiFilter,
                {},
                {},
                {
                    CollectionDescriptor::Grouped(ParameterType::MIDI_VALUE,CollectionType::GENERIC, 2)
                },
                0,
                0,
                1,
                1,
                false
            }
        },
        {
            ComponentType::BiquadFilter,
            {
                "Biquad Filter",
                ComponentType::BiquadFilter,
                {ParameterType::FREQUENCY,ParameterType::BANDWIDTH, ParameterType::Q_FACTOR, ParameterType::SHELF, ParameterType::DBGAIN},
                {ParameterType::FILTER_TYPE, ParameterType::FREQUENCY,ParameterType::BANDWIDTH, ParameterType::Q_FACTOR, ParameterType::SHELF, ParameterType::DBGAIN},
                {},
                1,
                1,
                0,
                0,
                true
            }
        },
        {
            ComponentType::Sequencer,
            {
                "Sequencer",
                ComponentType::Sequencer,
                {ParameterType::AMPLITUDE},
                {ParameterType::STATUS, ParameterType::BPM, ParameterType::DURATION},
                {
                    CollectionDescriptor::Synchronized(
                        {ParameterType::MIDI_VALUE, ParameterType::VELOCITY, ParameterType::START_POSITION, ParameterType::DURATION}, 
                        CollectionType::SEQUENCER
                    )
                },
                0,
                0,
                0,
                1,
                false
            }
        },
        {
            ComponentType::MonophonicFilter,
            {
                "Monophonic Filter",
                ComponentType::MonophonicFilter,
                {},
                {},
                {},
                0,
                0,
                1,
                1,
                false
            }
            
        },
        {
            ComponentType::Delay,
            {
                "Delay",
                ComponentType::Delay,
                {ParameterType::DELAY, ParameterType::GAIN},
                {ParameterType::DELAY, ParameterType::GAIN},
                {},
                1,
                1,
                0,
                0,
                false
            }
        },
        {
            ComponentType::Multiply,
            {
                "Multiply",
                ComponentType::Multiply,
                {ParameterType::SCALAR},
                {ParameterType::SCALAR},
                {},
                1,
                1,
                0,
                0,
                false
            }
        },
        {
            ComponentType::SpectrumAnalyzer,
            {
                "Spectrum Analyzer",
                ComponentType::SpectrumAnalyzer,
                {},
                {},
                {},
                1,
                0,
                0,
                0,
                false
            }
        }
    };

    return registry ;
}

const ComponentDescriptor& ComponentRegistry::getComponentDescriptor(ComponentType type){
    const auto& registry = ComponentRegistry::getAllComponentDescriptors();
    auto  it =  registry.find(type);
    if (it != registry.end()){
        return  it->second ;
    }
    SPDLOG_ERROR("Unknown ComponentType: {}", static_cast<int>(type));
    throw std::runtime_error("Unknown ComponentType");
}

const ComponentDescriptor& ComponentRegistry::getComponentDescriptor(std::string componentName){
    const auto& registry = ComponentRegistry::getAllComponentDescriptors();
    for ( const auto& [typ, d] : registry ){
        if ( d.name == componentName ){
            return d;
        }
    }
    SPDLOG_ERROR("Unknown ComponentName: {}", componentName);
    throw std::runtime_error("Unknown Component Name");
}