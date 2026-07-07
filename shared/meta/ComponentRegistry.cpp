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
#include "types/ParameterType.hpp"
#include <stdexcept>

const std::unordered_map<ComponentType, ComponentDescriptor>& ComponentRegistry::getAllComponentDescriptors(){
    static const std::unordered_map<ComponentType, ComponentDescriptor> registry = {
        {
            ComponentType::Oscillator,
            {
                .name = "Oscillator",
                .type = ComponentType::Oscillator,
                .modulatableParameters = {ParameterType::AMPLITUDE, ParameterType::FREQUENCY},
                .controllableParameters = {ParameterType::WAVEFORM, ParameterType::AMPLITUDE, ParameterType::FREQUENCY},
                .numSignalOutputs = 1,
                .canModulate = true
            }
        },
        {
            ComponentType::PolyOscillator, 
            {
                .name = "Polyphonic Oscillator",
                .type = ComponentType::PolyOscillator,
                .modulatableParameters = {ParameterType::AMPLITUDE, ParameterType::FREQUENCY, ParameterType::PHASE}, // modulatable params
                .controllableParameters = {ParameterType::DETUNE, ParameterType::WAVEFORM}, //control params
                .numSignalOutputs = 1, 
                .numMidiInputs = 1,
                .canModulate = false
            }
        },
        {
            ComponentType::LinearFader,
            {
                .name = "Linear Fader",
                .type = ComponentType::LinearFader,
                .modulatableParameters = {ParameterType::ATTACK, ParameterType::RELEASE},
                .controllableParameters = {ParameterType::ATTACK, ParameterType::RELEASE},
                .numMidiInputs = 1,
                .numMidiOutputs = 1,
                .canModulate = true
            }
        },
        {
            ComponentType::ADSREnvelope,
            {
                .name = "ADSR Envelope",
                .type = ComponentType::ADSREnvelope,
                .modulatableParameters = {ParameterType::ATTACK, ParameterType::DECAY, ParameterType::SUSTAIN, ParameterType::RELEASE},
                .controllableParameters = {ParameterType::ATTACK, ParameterType::DECAY, ParameterType::SUSTAIN, ParameterType::RELEASE, ParameterType::TRIGGER},
                .numMidiInputs = 1,
                .numMidiOutputs = 1,
                .canModulate = true
            }
        },
        {
            ComponentType::MidiFilter,
            {
                .name = "Midi Filter",
                .type = ComponentType::MidiFilter,
                .collection = CollectionDescriptor::Grouped(ParameterType::MIDI_VALUE, 2),
                .numMidiInputs = 1,
                .numMidiOutputs = 1,
                .canModulate = false
            }
        },
        {
            ComponentType::BiquadFilter,
            {
                .name = "Biquad Filter",
                .type = ComponentType::BiquadFilter,
                .modulatableParameters = {ParameterType::FREQUENCY,ParameterType::BANDWIDTH, ParameterType::Q_FACTOR, ParameterType::SHELF, ParameterType::DBGAIN},
                .controllableParameters = {ParameterType::FILTER_TYPE, ParameterType::FREQUENCY,ParameterType::BANDWIDTH, ParameterType::Q_FACTOR, ParameterType::SHELF, ParameterType::DBGAIN},
                .numSignalInputs = 1,
                .numSignalOutputs = 1,
                .canModulate = true
            }
        },
        {
            ComponentType::Sequencer,
            {
                .name = "Sequencer",
                .type = ComponentType::Sequencer,
                .controllableParameters = {ParameterType::STATUS, ParameterType::BPM, ParameterType::DURATION},
                .collection = CollectionDescriptor::Synchronized({
                    ParameterType::MIDI_VALUE, 
                    ParameterType::VELOCITY, 
                    ParameterType::START_POSITION, 
                    ParameterType::DURATION 
                }),
                .numMidiOutputs = 1,
            }
        },
        {
            ComponentType::MonophonicFilter,
            {
                .name = "Monophonic Filter",
                .type = ComponentType::MonophonicFilter,
                .numMidiInputs = 1,
                .numMidiOutputs = 1,
            }
        },
        {
            ComponentType::Delay,
            {
                .name = "Delay",
                .type = ComponentType::Delay,
                .modulatableParameters = {ParameterType::DELAY, ParameterType::GAIN},
                .controllableParameters = {ParameterType::DELAY, ParameterType::GAIN},
                .numSignalInputs = 1,
                .numSignalOutputs = 1,
            }
        },
        {
            ComponentType::Multiply,
            {
                .name = "Multiply",
                .type = ComponentType::Multiply,
                .modulatableParameters = {ParameterType::SCALAR},
                .controllableParameters = {ParameterType::SCALAR},
                .numSignalInputs = 1,
                .numSignalOutputs = 1,
            }
        },
        {
            ComponentType::SpectrumAnalyzer,
            {
                .name = "Spectrum Analyzer",
                .type = ComponentType::SpectrumAnalyzer,
                .numSignalInputs = 1,
            }
        },
        {
            ComponentType::Oscilloscope,
            {
                .name = "Oscilloscope",
                .type = ComponentType::Oscilloscope,
                .numSignalInputs = 1,
            }
        },
        {
            ComponentType::Panner,
            {
                .name = "Panner",
                .type = ComponentType::Panner,
                .modulatableParameters = {ParameterType::PAN},
                .controllableParameters = {ParameterType::PAN},
                .numSignalInputs = 1,
                .numSignalOutputs = 2,
            }
        },
        {
            ComponentType::FileBuffer,
            {
                .name = "File Buffer",
                .type = ComponentType::FileBuffer,
                .numBufferOutputs = 2,
                .hasFile = true 
            }
        },
        {
            ComponentType::BufferStreamer,
            {
                .name = "Buffer Streamer",
                .type = ComponentType::BufferStreamer,
                .controllableParameters = {ParameterType::STATUS},
                .numSignalOutputs = 1,
                .numBufferInputs = 1
            }
        },
        {
            ComponentType::Chopper,
            {
                .name = "Buffer Chopper",
                .type = ComponentType::Chopper,
                .collection = CollectionDescriptor::Grouped(ParameterType::SAMPLE, 2),
                .numBufferInputs = 1,
                .numBufferOutputs = 1,
                .allowMultipleBufferConnections = false,
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