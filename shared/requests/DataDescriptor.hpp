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

#ifndef DATA_DESCRIPTOR_HPP_
#define DATA_DESCRIPTOR_HPP_

#include <cstdint>
#include <string>

struct DataDescriptor {
    uint32_t componentId ;
    uint32_t channel ;
    uint64_t size ;

    std::string toString() const {
        return std::to_string(componentId) 
            + std::to_string(channel);
    }

    size_t hash() const {
        return std::hash<std::string>()(toString());
    }

    bool operator==(const DataDescriptor& other) const {
        return componentId == other.componentId && 
            channel == other.channel ;
    }
};

struct DataDescriptorHash {
    std::size_t operator()(const DataDescriptor& desc) const {
        return desc.hash() ;
    }
};

static_assert(sizeof(DataDescriptor) == 16);

#endif // DATA_DESCRIPTOR_HPP_