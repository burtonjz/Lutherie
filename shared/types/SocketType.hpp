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
#include <string_view>

enum class SocketType {
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

constexpr int N_SOCKET_TYPES = static_cast<int>(SocketType::N_SOCKET_TYPES) ;

constexpr std::array<std::string_view, N_SOCKET_TYPES> socketStrings({
    "Modulation Inbound",
    "Modulation Outbound",
    "Signal Inbound",
    "Signal Outbound",
    "MIDI Inbound",
    "MIDI Outbound",
    "Buffer Inbound",
    "Buffer Outbound"
});

const std::string socketType2String(SocketType s);
SocketType socketTypeFromString(std::string str);

#endif // __SHARED_SOCKET_TYPE_HPP_