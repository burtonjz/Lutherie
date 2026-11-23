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

#ifndef MIDI_CONNECTION_HPP_
#define MIDI_CONNECTION_HPP_

#include <cstddef>
#include <functional>
#include <unordered_set>

//forward declarations
class MidiIO ;

struct MidiConnection {
    MidiIO* midi ;
    size_t channel ;

    bool operator==(const MidiConnection& other) const {
        return midi == other.midi && channel == other.channel ;
    }
};

struct midiHash {
    size_t operator()(const MidiConnection& conn) const {
        return std::hash<MidiIO*>()(conn.midi) ^ (std::hash<size_t>()(conn.channel) << 1);
    }
};

using MidiConnectionSet = std::unordered_set<MidiConnection, midiHash> ;


#endif // MIDI_CONNECTION_HPP_

