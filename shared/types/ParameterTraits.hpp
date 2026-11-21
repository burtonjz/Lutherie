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

#ifndef PARAMETER_TRAITS_HPP_
#define PARAMETER_TRAITS_HPP_
 
#include "types/FilterType.hpp"
#include "types/ModulationStrategy.hpp"
#include "types/MonophonicTriggerType.hpp"
#include "types/ScaleNote.hpp"
#include "types/ScaleType.hpp"
#include "types/Waveform.hpp"
#include <cstdint>
#include <string_view>
#include <variant>

#define PARAMETER_TYPE_LIST \
    X(SCALAR) \
    X(DELAY) \
    X(DEPTH) \
    X(STATUS) \
    X(WAVEFORM) \
    X(FREQUENCY) \
    X(AMPLITUDE) \
    X(GAIN) \
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
    X(SCALE_TYPE) \
    X(TRIGGER) \
    X(SAMPLE) \
    X(LOOP) \
    X(PLAYBACK_RATE) \
    X(RECORD) 

namespace _parameter_detail {
    enum class Type : uint8_t {
        #define X(NAME) NAME,
        PARAMETER_TYPE_LIST
        #undef X
        N_PARAMETER_TYPES
    };
};


/*
====================VARIANT PARAMETER VALUE======================
*/
using ParameterValue = std::variant<bool, uint8_t, int, float, double> ;

namespace nlohmann {
template <>
struct adl_serializer<ParameterValue> {
    static void to_json(json& j, const ParameterValue& value){
        std::visit([&j](const auto& v){
            using T = std::decay_t<decltype(v)> ;

            if constexpr ( std::is_same_v<T, bool> ){
                j["value"] = v ;
                j["type"] = "bool" ;
            } else if constexpr ( std::is_same_v<T, uint8_t> ){
                j["value"] = static_cast<int>(v) ;
                j["type"] = "uint8" ;
            } else if constexpr ( std::is_same_v<T, int> ){
                j["value"] = v ;
                j["type"] = "int" ;
            } else if constexpr ( std::is_same_v<T, float> ){
                j["value"] = v ;
                j["type"] = "float" ;
            } else if constexpr ( std::is_same_v<T, double> ){
                j["value"] = v ;
                j["type"] = "double" ;
            }
        }, value);
    }

    static ParameterValue from_json(const json& j) {
        const auto& type = j.at("type");
        const auto& v = j.at("value");

        if ( type == "bool" )        return v.get<bool>();
        else if ( type == "uint8" )  return static_cast<uint8_t>(v.get<int>());
        else if ( type == "int" )    return v.get<int>();
        else if ( type == "float" )  return v.get<float>();
        else if ( type == "double" ) return v.get<double>();

        throw json::type_error::create(
            302, 
            "Invalid ParameterValue type", 
            &j
        );
    }
};
}

/*
====================PARAMETER TYPE TRAITS======================
*/
template <_parameter_detail::Type T> struct ParameterTraits ;

template <> struct ParameterTraits<_parameter_detail::Type::SCALAR>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::SCALAR ;
    using ValueType = float ;

    static constexpr std::string_view name = "scalar" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ; 
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ; 
    };

    struct Defaults {
        static constexpr ValueType value   = 1.0f ; 
        static constexpr ValueType minimum = -1.0f ;
        static constexpr ValueType maximum = 1.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::ADDITIVE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = -1.0f ;
        static constexpr ValueType maximum = 1.0f ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::DELAY>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::DELAY ;
    using ValueType = int ;

    static constexpr std::string_view name = "delay" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0 ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = 48000 * 4 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::ADDITIVE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = 48000 * 4 ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::DEPTH>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::DEPTH ;
    using ValueType = double ;

    static constexpr std::string_view name = "depth" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 1.0 ;
        static constexpr ValueType minimum = -10.0 ;
        static constexpr ValueType maximum = 10.0 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::ADDITIVE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = -10.0 ;
        static constexpr ValueType maximum = 10.0 ;
    };

    struct UI {
        static constexpr size_t precision = 2 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::STATUS>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::STATUS ;
    using ValueType = bool ;

    static constexpr std::string_view name = "status" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = true ;
        static constexpr ValueType minimum = false ;
        static constexpr ValueType maximum = true ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = false ;
        static constexpr ValueType maximum = true ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::WAVEFORM>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::WAVEFORM ;
    using ValueType = uint8_t ;

    static constexpr std::string_view name = "waveform" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = Waveform::SINE ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = Waveform::count ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = Waveform::count ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::FREQUENCY>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::FREQUENCY ;
    using ValueType = double ;

    static constexpr std::string_view name = "frequency" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 440.0 ;
        static constexpr ValueType minimum = 0.0 ;
        static constexpr ValueType maximum = 30000.0 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.0 ;
        static constexpr ValueType maximum = 30000.0 ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::AMPLITUDE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::AMPLITUDE ;
    using ValueType = double ;

    static constexpr std::string_view name = "amplitude" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 1.0 ;
        static constexpr ValueType minimum = 0.0 ;
        static constexpr ValueType maximum = 1.0 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::MULTIPLICATIVE_ZERO},
            {ModulatorRange::BIPOLAR, ModulationStrategy::MULTIPLICATIVE_UNITY},
            {ModulatorRange::UNKNOWN, ModulationStrategy::MULTIPLICATIVE_ZERO},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.0 ;
        static constexpr ValueType maximum = 1.0 ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::GAIN>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::GAIN ;
    using ValueType = double ;

    static constexpr std::string_view name = "gain" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 1.0 ;
        static constexpr ValueType minimum = 0.0 ;
        static constexpr ValueType maximum = 16.0 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::MULTIPLICATIVE_ZERO},
            {ModulatorRange::BIPOLAR, ModulationStrategy::MULTIPLICATIVE_UNITY},
            {ModulatorRange::UNKNOWN, ModulationStrategy::MULTIPLICATIVE_ZERO},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.0 ;
        static constexpr ValueType maximum = 16.0 ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
        static constexpr size_t precisionDb = 1 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::PHASE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::PHASE ;
    using ValueType = double ;

    static constexpr std::string_view name = "phase" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 1.0 ;
        static constexpr ValueType minimum = 0.0 ;
        static constexpr ValueType maximum = 1.0 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::ADDITIVE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.0 ;
        static constexpr ValueType maximum = 1.0 ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::PAN>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::PAN ;
    using ValueType = float ;

    static constexpr std::string_view name = "pan" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0.0f ;
        static constexpr ValueType minimum = -1.0f ;
        static constexpr ValueType maximum = 1.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::ADDITIVE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = -1.0f ;
        static constexpr ValueType maximum = 1.0f ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::ATTACK>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::ATTACK ;
    using ValueType = float ;

    static constexpr std::string_view name = "attack" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0.01f ;
        static constexpr ValueType minimum = 0.001f ;
        static constexpr ValueType maximum = 4.00f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.001f ;
        static constexpr ValueType maximum = 4.00f ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::DECAY>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::DECAY ;
    using ValueType = float ;

    static constexpr std::string_view name = "decay" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0.01f ;
        static constexpr ValueType minimum = 0.001f ;
        static constexpr ValueType maximum = 4.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.001f ;
        static constexpr ValueType maximum = 4.0f ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::SUSTAIN>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::SUSTAIN ;
    using ValueType = float ;

    static constexpr std::string_view name = "sustain" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0.8f ;
        static constexpr ValueType minimum = 0.0f ;
        static constexpr ValueType maximum = 1.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::ADDITIVE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.0f ;
        static constexpr ValueType maximum = 1.0f ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::RELEASE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::RELEASE ;
    using ValueType = float ;

    static constexpr std::string_view name = "release" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0.01f ;
        static constexpr ValueType minimum = 0.001f ;
        static constexpr ValueType maximum = 4.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.001f ;
        static constexpr ValueType maximum = 4.0f ;
    };

    struct UI {
        static constexpr size_t precision = 3 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::DETUNE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::DETUNE ;
    using ValueType = double ;

    static constexpr std::string_view name = "detune" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0.0 ;
        static constexpr ValueType minimum = -4800.0 ;
        static constexpr ValueType maximum = 4800.0 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = -4800.0 ;
        static constexpr ValueType maximum = 4800.0 ;
    };

    struct UI {
        static constexpr size_t precision = 2 ;

        static constexpr float harmonicMin = 0.5f ;
        static constexpr float harmonicMax = 8.0f ;
        static constexpr size_t harmonicPrecision = 2 ;
        static constexpr float detuneCentsMin = -100.0f ;
        static constexpr float detuneCentsMax = 100.0f ;
        static constexpr size_t detunePrecision = 1 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::FILTER_TYPE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::FILTER_TYPE ;
    using ValueType = uint8_t ;

    static constexpr std::string_view name = "filter type" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = FilterType::LowPass ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = FilterType::count ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = FilterType::count ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::CUTOFF>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::CUTOFF ;
    using ValueType = float ;

    static constexpr std::string_view name = "cutoff" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 20000.0f ;
        static constexpr ValueType minimum = 0.0f ;
        static constexpr ValueType maximum = 30000.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.0f ;
        static constexpr ValueType maximum = 30000.0f ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::BANDWIDTH>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::BANDWIDTH ;
    using ValueType = float ;

    static constexpr std::string_view name = "bandwidth" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 2.0f ;
        static constexpr ValueType minimum = 0.1f ;
        static constexpr ValueType maximum = 4.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.1f ;
        static constexpr ValueType maximum = 4.0f ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::SHELF>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::SHELF ;
    using ValueType = float ;

    static constexpr std::string_view name = "shelf slope" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 1.0f ;
        static constexpr ValueType minimum = 0.1f ;
        static constexpr ValueType maximum = 2.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.1f ;
        static constexpr ValueType maximum = 2.0f ;
    };

    struct UI {
        static constexpr size_t precision = 2 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::Q_FACTOR>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::Q_FACTOR ;
    using ValueType = float ;

    static constexpr std::string_view name = "q factor" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0.5f ;
        static constexpr ValueType minimum = 0.5f ;
        static constexpr ValueType maximum = 10.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.5f ;
        static constexpr ValueType maximum = 10.0f ;
    };

    struct UI {
        static constexpr size_t precision = 2 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::BPM>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::BPM ;
    using ValueType = int ;

    static constexpr std::string_view name = "bpm" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 120 ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = 1000 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::ADDITIVE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::ADDITIVE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = 1000 ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::MIDI_VALUE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::MIDI_VALUE ;
    using ValueType = uint8_t ;

    static constexpr std::string_view name = "midi value" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 69 ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = 127 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = 127 ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::VELOCITY>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::VELOCITY ;
    using ValueType = uint8_t ;

    static constexpr std::string_view name = "midi velocity" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 100 ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = 127 ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = 127 ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::START_POSITION>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::START_POSITION ;
    using ValueType = float ;

    static constexpr std::string_view name = "start" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0.0f ;
        static constexpr ValueType minimum = 0.0f ;
        static constexpr ValueType maximum = 64.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.0f ;
        static constexpr ValueType maximum = 64.0f ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::DURATION>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::DURATION ;
    using ValueType = float ;

    static constexpr std::string_view name = "duration" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 1.0f ;
        static constexpr ValueType minimum = 0.0f ;
        static constexpr ValueType maximum = 64.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.0f ;
        static constexpr ValueType maximum = 64.0f ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::SCALE_NOTE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::SCALE_NOTE ;
    using ValueType = uint8_t ;

    static constexpr std::string_view name = "note" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = ScaleNote::C ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = ScaleNote::count ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = ScaleNote::count ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::SCALE_TYPE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::SCALE_TYPE ;
    using ValueType = uint8_t ;

    static constexpr std::string_view name = "scale type" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = ScaleType::MAJOR ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = ScaleType::count ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = ScaleType::count ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::TRIGGER>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::TRIGGER ;
    using ValueType = uint8_t ;

    static constexpr std::string_view name = "retrigger" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = MonophonicTriggerType::LEGATO ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = MonophonicTriggerType::count ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = MonophonicTriggerType::count ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::SAMPLE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::SAMPLE ;
    using ValueType = int ;

    static constexpr std::string_view name = "sample" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 0 ;
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = std::numeric_limits<int>::max() ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0 ;
        static constexpr ValueType maximum = std::numeric_limits<int>::max() ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::LOOP>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::LOOP ;
    using ValueType = bool ;

    static constexpr std::string_view name = "loop" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = false ;
        static constexpr ValueType minimum = false ;
        static constexpr ValueType maximum = true ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = false ;
        static constexpr ValueType maximum = true ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::PLAYBACK_RATE>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::PLAYBACK_RATE ;
    using ValueType = float ;

    static constexpr std::string_view name = "playback rate" ;
    struct Supports {
        static constexpr bool rangeUpdate = true ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = 1.0f ;
        static constexpr ValueType minimum = 0.05f ;
        static constexpr ValueType maximum = 8.0f ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::BIPOLAR, ModulationStrategy::EXPONENTIAL},
            {ModulatorRange::UNKNOWN, ModulationStrategy::EXPONENTIAL},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = 0.05f ;
        static constexpr ValueType maximum = 8.0f ;
    };

    struct UI {
        static constexpr size_t precision = 2 ;
    };
};

template <> struct ParameterTraits<_parameter_detail::Type::RECORD>{
    static constexpr _parameter_detail::Type type = _parameter_detail::Type::RECORD ;
    using ValueType = bool ;

    static constexpr std::string_view name = "record" ;
    struct Supports {
        static constexpr bool rangeUpdate = false ;
        static constexpr bool modulatable = true ;
        static constexpr bool multiValue  = true ;
    };

    struct Defaults {
        static constexpr ValueType value   = false ;
        static constexpr ValueType minimum = false ;
        static constexpr ValueType maximum = true ;

        static constexpr std::array<std::pair<ModulatorRange,ModulationStrategy>, 3> modulationStrategy = {{
            {ModulatorRange::UNIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::BIPOLAR, ModulationStrategy::NONE},
            {ModulatorRange::UNKNOWN, ModulationStrategy::NONE},
        }};
    };

    struct Limits {
        static constexpr ValueType minimum = false ;
        static constexpr ValueType maximum = true ;
    };

    struct UI {
        static constexpr size_t precision = 0 ;
    };
};

#endif // PARAMETER_TRAITS_HPP_