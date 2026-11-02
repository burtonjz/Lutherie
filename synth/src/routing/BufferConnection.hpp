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

#ifndef BUFFER_CONNECTION_HPP_
#define BUFFER_CONNECTION_HPP_

#include <cstddef>
#include <functional>
#include <unordered_set>

//forward declarations
class BufferIO ;

struct BufferConnection {
    BufferIO* component ;
    size_t index ;

    bool operator==(const BufferConnection& other) const {
        return component == other.component && index == other.index ;
    }
};

struct BufferHash {
    std::size_t operator()(const BufferConnection& conn) const {
        return std::hash<BufferIO*>()(conn.component) ^ (std::hash<size_t>()(conn.index) << 1);
    }
};

using BufferConnectionSet = std::unordered_set<BufferConnection, BufferHash> ;

#endif // BUFFER_CONNECTION_HPP_