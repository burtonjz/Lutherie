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

#ifndef __SHARED_SOCKET_TYPE_HPP_
#define __SHARED_SOCKET_TYPE_HPP_

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;
class SocketType {
public:
    enum Value : uint8_t {
        ModulationInbound,
        ModulationOutbound,
        SignalInbound,
        SignalOutbound,
        MidiInbound,
        MidiOutbound,
        BufferInbound,
        BufferOutbound,
        N_SOCKET_TYPES
    };

    SocketType() = default ;
    constexpr SocketType(Value v) : value_(v){}

    constexpr operator Value() const { return value_ ; }

    std::string toString() const {
        return std::string(names_[value_]);
    }

    static SocketType fromString(std::string_view str){
        for (int i = 0; i < N_SOCKET_TYPES; ++i){
            if ( names_[i] == str){
                return static_cast<Value>(i);
            }
        }   
        throw std::invalid_argument("unknown SocketType: " + std::string(str));
    }

    uint8_t to_uint8() const {
        return value_ ;
    }

    static SocketType from_uint8(uint8_t val){
        return SocketType(static_cast<Value>(val));
    }

    static const std::array<std::string_view, N_SOCKET_TYPES>& getNames(){
        return names_ ;
    }

    static constexpr int count = N_SOCKET_TYPES ;

    constexpr bool isInbound() const {
        switch (value_){
            case ModulationInbound:
            case SignalInbound:
            case MidiInbound:
            case BufferInbound:
                return true;
            default:
                return false;
        }
    }

    constexpr bool isAudio() const {
        switch (value_){
            case SignalInbound:
            case SignalOutbound:
            case BufferInbound:
            case BufferOutbound:
                return true ;
            default:
                return false ;
        }
    }

    const SocketType getMatchingType() const {
        switch (value_){
            case ModulationInbound:
                return ModulationOutbound ;
            case ModulationOutbound:
                return ModulationInbound ;
            case SignalInbound:
                return SignalOutbound ;
            case SignalOutbound:
                return SignalInbound ;
            case MidiInbound:
                return MidiOutbound ;
            case MidiOutbound:
                return MidiInbound ;
            case BufferInbound:
                return BufferOutbound ;
            case BufferOutbound:
                return BufferInbound ;
            default:
                throw std::runtime_error("SocketType case not defined.");
        }
    }

private:
    Value value_{ModulationInbound};

    static constexpr std::array<std::string_view, N_SOCKET_TYPES> names_{
        "Modulation Inbound", 
        "Modulation Outbound",
        "Signal Inbound",     
        "Signal Outbound",
        "MIDI Inbound",       
        "MIDI Outbound",
        "Buffer Inbound",     
        "Buffer Outbound"
    };
};

inline void from_json(const json& j, SocketType& t) {
    t = SocketType::fromString(j.get_ref<const std::string&>());
}

inline void to_json(json& j, const SocketType& t) {
    j = t.toString();
}

#endif // __SHARED_SOCKET_TYPE_HPP_