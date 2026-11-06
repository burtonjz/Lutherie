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

#ifndef AUDIO_CONNECTION_HPP_
#define AUDIO_CONNECTION_HPP_

#include <cstddef>
#include <functional>
#include <unordered_set>

// forward declarations 
class AudioIO ;

struct AudioConnection {
    AudioIO* audio ; 
    size_t channel ; 

    bool operator==(const AudioConnection& other) const {
        return audio == other.audio && channel == other.channel ;
    }
};

struct ConnectionHash {
    size_t operator()(const AudioConnection& conn) const {
        return std::hash<AudioIO*>()(conn.audio) ^ (std::hash<size_t>()(conn.channel) << 1);
    }
};

using AudioConnectionSet = std::unordered_set<AudioConnection, ConnectionHash> ;

#endif // AUDIO_CONNECTION_HPP_
