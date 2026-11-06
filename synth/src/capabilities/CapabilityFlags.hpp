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

#ifndef CAPABILITY_FLAGS_HPP_
#define CAPABILITY_FLAGS_HPP_

#include <cstdint>

enum class CapabilityFlags : uint32_t {
    None         = 0,
    Parameters   = 1u << 0,
    AudioIO      = 1u << 1,
    BufferIO     = 1u << 2,
    MidiHandler  = 1u << 3,
    MidiListener = 1u << 4,
    Modulator    = 1u << 5,
    AudioProbe   = 1u << 6,
    File         = 1u << 7
};

constexpr CapabilityFlags operator|(CapabilityFlags a, CapabilityFlags b){
    return static_cast<CapabilityFlags>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

constexpr CapabilityFlags operator&(CapabilityFlags a, CapabilityFlags b) {
    return static_cast<CapabilityFlags>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
    );
}

constexpr CapabilityFlags& operator|=(CapabilityFlags& a, CapabilityFlags b) {
    return a = a | b ;
}

constexpr bool hasCap(CapabilityFlags flags, CapabilityFlags cap) {
    return (flags & cap) != CapabilityFlags::None ;
}

constexpr CapabilityFlags& operator&=(CapabilityFlags& a, CapabilityFlags b) {
    return a = a & b ;
}

#endif // CAPABILITY_FLAGS_HPP_