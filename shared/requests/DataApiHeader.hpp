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

#ifndef DATA_API_HEADER_HPP_
#define DATA_API_HEADER_HPP_

#include "types/NodeType.hpp"
#include <cstdint>
#include <string>

struct DataApiHeader {
    NodeType type ;
    uint32_t id ;
    uint32_t channel ;
    uint64_t size = 0 ;

    std::string toString() const {
        return type.toString() 
            + std::to_string(id) 
            + std::to_string(channel);
    }

    size_t hash() const {
        return std::hash<std::string>()(toString());
    }

    bool operator==(const DataApiHeader& other) const {
        return type == other.type
            && id == other.id
            && channel == other.channel ;
    }
};

struct DataDescriptorHash {
    size_t operator()(const DataApiHeader& desc) const {
        return desc.hash() ;
    }
};

static_assert(sizeof(DataApiHeader) == 24);

#endif // DATA_API_HEADER_HPP_