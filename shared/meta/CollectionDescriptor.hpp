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

#ifndef __COLLECTION_DESCRIPTOR_HPP_
#define __COLLECTION_DESCRIPTOR_HPP_

#include "types/ParameterType.hpp"

#include <vector>

enum class CollectionStructure {
    INDEPENDENT, // each element may be updated independently
    GROUPED, // elements within collection are treated as groups of N
    SYNCHRONIZED // multiple collections must have same length and parallel indices
};

class CollectionDescriptor {
private:
    std::vector<ParameterType> parameters_ ;
    CollectionStructure structure_ ;
    size_t groupSize_ ; // for GROUPED only
    size_t initialCount_ ;

    CollectionDescriptor(
        std::vector<ParameterType> parameters,
        CollectionStructure structure,
        size_t initialCount = 0, size_t groupSize = 1
    );

public:
    static CollectionDescriptor Independent(
        ParameterType p, 
        size_t initialCount = 0
    );

    static CollectionDescriptor Grouped(
        ParameterType p,
        size_t groupSize,
        size_t initialCount = 0
    );

    static CollectionDescriptor Synchronized(
        std::vector<ParameterType> params,
        size_t initialCount = 0
    );

    std::span<const ParameterType> parameters() const ;
    CollectionStructure structure() const ;
    size_t groupSize() const ;
    size_t initialCount() const ;

private:
    bool isValid() const ;

};

struct CollectionHash {
    size_t operator()(const CollectionDescriptor& collection) const ;
};

// JSON (de)serialization
namespace nlohmann {
template <>
struct adl_serializer<CollectionDescriptor> {
    static CollectionDescriptor from_json(const json& j);

    static void to_json(json& j, const CollectionDescriptor& cd);
};
} 

#endif // __COLLECTION_DESCRIPTOR_HPP_