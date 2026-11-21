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

#include <stdexcept>
#include <vector>

enum class CollectionStructure {
    INDEPENDENT, // each element may be updated independently
    GROUPED, // elements within collection are treated as groups of N
    SYNCHRONIZED // multiple collections must have same length and parallel indices
};

struct CollectionDescriptor {
    std::vector<ParameterType> parameters ;
    CollectionStructure structure ;
    size_t groupSize = 0 ; // for GROUPED only

    static CollectionDescriptor Independent(
        ParameterType p
    ){
        return {
            .parameters={p},
            .structure=CollectionStructure::INDEPENDENT,
            .groupSize=0
        };
    }

    static CollectionDescriptor Grouped(
        ParameterType p,
        size_t groupSize
    ){
        if (groupSize < 2){
            throw std::invalid_argument("Grouped collections must have size > 1");
        }
        return {
            .parameters={p},
            .structure=CollectionStructure::GROUPED,
            .groupSize=groupSize
        };
    }

    static CollectionDescriptor Synchronized(
        const std::vector<ParameterType>& params
    ){
        if ( params.size() < 2 ){
            throw std::invalid_argument("Synchronized collections require at least 2 parameter types");
        }
        return {
            .parameters=params,
            .structure=CollectionStructure::SYNCHRONIZED,
        };
    }

    bool isValid() const {
        switch(structure){
        case CollectionStructure::INDEPENDENT:
            return parameters.size() == 1 ;
        case CollectionStructure::GROUPED:
            return parameters.size() == 1 && groupSize > 1 ;
        case CollectionStructure::SYNCHRONIZED:
            return parameters.size() > 1 ;
        default:
            return false ;
        }
    }
};

struct CollectionHash {
    size_t operator()(const CollectionDescriptor& collection) const {
        size_t seed = 0 ;

        // Fibonacci Hashing
        auto hashCombine = [&seed](size_t value) {
            seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        hashCombine(std::hash<int>{}(
            static_cast<int>(collection.structure)
        ));

        for (const auto& param : collection.parameters) {
            hashCombine(std::hash<uint8_t>{}(param.to_uint8()));
        }

        if (collection.structure == CollectionStructure::GROUPED) {
            hashCombine(std::hash<size_t>{}(collection.groupSize));
        }

        return seed ;
    }
};

// JSON (de)serialization
namespace nlohmann {
template <>
struct adl_serializer<CollectionDescriptor> {
    static CollectionDescriptor from_json(const json& j){
        return CollectionDescriptor {
            .parameters = j.at("parameters"),
            .structure = j.at("structure"),
            .groupSize = j.at("groupSize")
        };
    }

    static void to_json(json& j, const CollectionDescriptor& req){
        j["parameters"] = req.parameters ;
        j["structure"] = req.structure ;
        j["groupSize"] = req.groupSize ;
    }
};
} 

#endif // __COLLECTION_DESCRIPTOR_HPP_