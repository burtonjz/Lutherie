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

#ifndef __PARAMETER_TYPE_HPP_
#define __PARAMETER_TYPE_HPP_

#include "types/FilterType.hpp"
#include "types/Waveform.hpp"
#include "types/ScaleType.hpp"
#include "types/ScaleNote.hpp"

#include <variant>
#include <cstdint>

using ParameterValue = std::variant<bool, uint8_t, int, float, double> ;
using json = nlohmann::json ;

json ParameterValueToJson(const ParameterValue& v);


/*
To add a ParameterType, the following is required:
1. add it to the X Macro PARAMETER_TYPE_LIST
2. Define the specialized ParameterTraits struct

*/ 

// X-Macro for generating dispatch functions (see synth/src/params/ParameterMap.cpp for examples)
#define PARAMETER_TYPE_LIST \
    X(SCALAR) \
    X(DELAY) \
    X(DEPTH) \
    X(STATUS) \
    X(WAVEFORM) \
    X(FREQUENCY) \
    X(AMPLITUDE) \
    X(GAIN) \
    X(DBGAIN) \
    X(PHASE) \
    X(PAN) \
    X(DETUNE) \
    X(ATTACK) \
    X(DECAY) \
    X(SUSTAIN) \
    X(RELEASE) \
    X(FILTER_TYPE) \
    X(CUTOFF) \
    X(BANDWIDTH) \
    X(SHELF) \
    X(Q_FACTOR) \
    X(BPM) \
    X(MIDI_VALUE) \
    X(VELOCITY) \
    X(START_POSITION) \
    X(DURATION) \
    X(SCALE_NOTE) \
    X(SCALE_TYPE)

/**
 * @brief types of parameters that might exist within any given module
 * 
*/
enum class ParameterType: uint8_t {
    #define X(name) \
        name,
        PARAMETER_TYPE_LIST
    #undef X
    N_PARAMETERS
};

constexpr int N_PARAMETER_TYPES = static_cast<int>(ParameterType::N_PARAMETERS) ;

ParameterType parameterFromString(std::string str);

/* 
every ParameterType will store a default modulation strategy
based on the type of variable it is. This can be overridden in
a Parameter instance. 
*/ 
#define MODULATION_STRATEGY_LIST \
    X(ADDITIVE) \
    X(MULTIPLICATIVE) \
    X(EXPONENTIAL) \
    X(LOGARITHMIC) \
    X(REPLACE) \
    X(NONE) 

enum class ModulationStrategy : uint8_t {
    #define X(name) \
        name,
        MODULATION_STRATEGY_LIST
    #undef X
    N_STRATEGIES
};

ModulationStrategy modStrategyFromString(std::string str);
std::string modStrategyToString(ModulationStrategy str);

// Parameter Traits access
template <ParameterType Type> struct ParameterTraits ;


template <> struct ParameterTraits<ParameterType::SCALAR>{
    using ValueType = float ;
    static constexpr std::string name = "scalar" ;
    static constexpr float minimum = -1.0 ;
    static constexpr float maximum = 1.0 ;
    static constexpr float defaultValue = 1.0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::ADDITIVE ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::DELAY>{
    using ValueType = int ; // number of samples
    static constexpr std::string name = "delay" ;
    static constexpr float minimum = 0 ;
    static constexpr float maximum = 48000 * 4.0 ;
    static constexpr float defaultValue = 0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::ADDITIVE ;
    static constexpr size_t uiPrecision = 0 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::DEPTH>{
    using ValueType = float ;
    static constexpr std::string name = "depth" ;
    static constexpr float minimum = -1.0 ;
    static constexpr float maximum = 1.0 ;
    static constexpr float defaultValue = 1.0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::ADDITIVE ;
    static constexpr size_t uiPrecision = 2 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::STATUS>{
    using ValueType = bool ;
    static constexpr std::string name = "status" ;
    static constexpr float minimum = 0 ;
    static constexpr float maximum = 1 ;
    static constexpr float defaultValue = 1 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::NONE ;
    static constexpr size_t uiPrecision = 0 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::WAVEFORM>{
    using ValueType = uint8_t ;
    static constexpr std::string name = "waveform" ;
    static constexpr float minimum = 0 ;
    static constexpr float maximum = Waveform::N ;
    static constexpr float defaultValue = Waveform::SINE ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::NONE ;
    static constexpr size_t uiPrecision = 0 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::FREQUENCY>{
    using ValueType = double ;
    static constexpr std::string name = "frequency" ;
    static constexpr float minimum = 0.0 ;
    static constexpr float maximum = 30000.0 ;
    static constexpr float defaultValue = 440.0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::EXPONENTIAL ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::AMPLITUDE>{
    using ValueType = double ;
    static constexpr std::string name = "amplitude" ;
    static constexpr float minimum = 0.0 ;
    static constexpr float maximum = 1.0 ;
    static constexpr float defaultValue = 1.0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::LOGARITHMIC ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::GAIN>{
    using ValueType = double ;
    static constexpr std::string name = "gain" ;
    static constexpr float minimum = 0.0 ;
    static constexpr float maximum = 1.0 ;
    static constexpr float defaultValue = 1.0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::LOGARITHMIC ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::DBGAIN>{
    using ValueType = double ;
    static constexpr std::string name = "gain (db)" ;
    static constexpr float minimum = -24.0 ;
    static constexpr float maximum = 24.0 ;
    static constexpr float defaultValue = 0.0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::ADDITIVE ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::PHASE>{
    using ValueType = double ;
    static constexpr std::string name = "phase" ;
    static constexpr float minimum = 0.0 ;
    static constexpr float maximum = 1.0 ;
    static constexpr float defaultValue = 1.0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::ADDITIVE ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::PAN>{
    using ValueType = float ;
    static constexpr std::string name = "pan" ;
    static constexpr float minimum = -1.0 ;
    static constexpr float maximum = 1.0 ;
    static constexpr float defaultValue = 0.0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::ADDITIVE ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::DETUNE>{
    using ValueType = float ;
    static constexpr std::string name = "detune" ;
    static constexpr float minimum = -1250.0f ;
    static constexpr float maximum = 1250.0f ;
    static constexpr float defaultValue = 0.0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::EXPONENTIAL ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::ATTACK>{
    using ValueType = float ;
    static constexpr std::string name = "attack" ;
    static constexpr float minimum = 0.001 ;
    static constexpr float maximum = 4.00 ;
    static constexpr float defaultValue = 0.01 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::EXPONENTIAL ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::DECAY>{
    using ValueType = float ;
    static constexpr std::string name = "decay" ;
    static constexpr float minimum = 0.001 ;
    static constexpr float maximum = 4.0 ;
    static constexpr float defaultValue = 0.01 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::EXPONENTIAL ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::SUSTAIN>{
    using ValueType = float ;
    static constexpr std::string name = "sustain" ;
    static constexpr float minimum = 0.0 ;
    static constexpr float maximum = 1.0 ;
    static constexpr float defaultValue = 0.8 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::ADDITIVE ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::RELEASE>{
    using ValueType = float ;
    static constexpr std::string name = "release" ;
    static constexpr float minimum = 0.0 ;
    static constexpr float maximum = 4.0 ;
    static constexpr float defaultValue = 0.01 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::EXPONENTIAL ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::FILTER_TYPE>{
    using ValueType = uint8_t ;
    static constexpr std::string name = "filter type" ;
    static constexpr float minimum = 0 ;
    static constexpr float maximum = FilterType::N ;
    static constexpr float defaultValue = FilterType::LowPass ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::NONE ;
    static constexpr size_t uiPrecision = 0 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::CUTOFF>{
    using ValueType = float ;
    static constexpr std::string name = "cutoff" ;
    static constexpr float minimum = 0.0f ;
    static constexpr float maximum = 30000.0f ;
    static constexpr float defaultValue = 20000.0f ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::EXPONENTIAL ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::BANDWIDTH>{
    using ValueType = float ;
    static constexpr std::string name = "bandwidth" ;
    static constexpr float minimum = 0.1f ;
    static constexpr float maximum = 4.0f ;
    static constexpr float defaultValue = 2.0f ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::EXPONENTIAL ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::SHELF>{
    using ValueType = float ;
    static constexpr std::string name = "shelf slope" ;
    static constexpr float minimum = 0.1f ;
    static constexpr float maximum = 2.0f ;
    static constexpr float defaultValue = 1.0f ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::EXPONENTIAL ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::Q_FACTOR>{
    using ValueType = float ;
    static constexpr std::string name = "q factor" ;
    static constexpr float minimum = 0.5f ;
    static constexpr float maximum = 10.0f ;
    static constexpr float defaultValue = 0.5f ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::EXPONENTIAL ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::BPM>{
    using ValueType = int ;
    static constexpr std::string name = "bpm";
    static constexpr float minimum = 0 ;
    static constexpr float maximum = 300 ;
    static constexpr float defaultValue = 120 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::MULTIPLICATIVE ;
    static constexpr size_t uiPrecision = 0 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::MIDI_VALUE>{
    using ValueType = uint8_t ;
    static constexpr std::string name = "midi value" ;
    static constexpr float minimum = 0 ;
    static constexpr float maximum = 127 ;
    static constexpr float defaultValue = 69 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::NONE ;
    static constexpr size_t uiPrecision = 0 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::VELOCITY>{
    using ValueType = uint8_t ;
    static constexpr std::string name = "velocity" ;
    static constexpr float minimum = 0 ;
    static constexpr float maximum = 127 ;
    static constexpr float defaultValue = 100 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::NONE ;
    static constexpr size_t uiPrecision = 0 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::START_POSITION>{
    using ValueType = float ;
    static constexpr std::string name = "start";
    static constexpr float minimum = 0 ;
    static constexpr float maximum = 64 ;
    static constexpr float defaultValue = 0 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::NONE ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::DURATION>{
    using ValueType = float ;
    static constexpr std::string name = "duration" ;
    static constexpr float minimum = 0 ;
    static constexpr float maximum = 64 ;
    static constexpr float defaultValue = 1 ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::NONE ;
    static constexpr size_t uiPrecision = 3 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::SCALE_NOTE>{
    using ValueType = uint8_t ;
    static constexpr std::string name = "Note" ;
    static constexpr float minimum = 0 ;
    static constexpr float maximum = ScaleNote::N ;
    static constexpr float defaultValue = ScaleNote::C ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::NONE ;
    static constexpr size_t uiPrecision = 0 ; // num decimals
};

template <> struct ParameterTraits<ParameterType::SCALE_TYPE>{
    using ValueType = uint8_t ;
    static constexpr std::string name = "Scale Type" ;
    static constexpr float minimum = 0 ;
    static constexpr float maximum = ScaleType::N ;
    static constexpr float defaultValue = ScaleType::MAJOR ;
    static constexpr ModulationStrategy defaultStrategy = ModulationStrategy::NONE ;
    static constexpr size_t uiPrecision = 0 ; // num decimals
};
  
/*
The following dispatch function and macro allows users to easily retreive a trait for a particular parameter at runtime

EX:
ParameterType p = ParameterType::FREQUENCY ;
auto defaultFrequency = GET_PARAMETER_TRAIT_MEMBER(p, defaultValue);
*/ 

template <typename Func>
auto dispatchParameterTrait(ParameterType p, Func&& func){
     switch(p) { 
        #define X(NAME) case ParameterType::NAME: return func(ParameterTraits<ParameterType::NAME>{});
        PARAMETER_TYPE_LIST
        #undef X
        default: throw std::runtime_error("Unknown ParameterType");
    }
} 

#define GET_PARAMETER_VALUE_TYPE(type) \
    typename ParameterTraits<type>::ValueType 
    
#define GET_PARAMETER_TRAIT_MEMBER(type, member) \
    dispatchParameterTrait(type, [](auto traits) { return decltype(traits)::member ; })


#endif // __PARAMETER_TYPE_HPP_