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

#include "meta/CollectionDescriptor.hpp"
#include <initializer_list>
#include <nlohmann/adl_serializer.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

CollectionDescriptor::CollectionDescriptor(
    std::vector<ParameterType> parameters,
    CollectionStructure structure,
    size_t initialCount, size_t groupSize
):
    parameters_(std::move(parameters)),
    structure_(structure),
    groupSize_(groupSize),
    initialCount_(initialCount)
{
    if ( !isValid() ){
        auto msg = "invalid collection descriptor" ;
        SPDLOG_ERROR(msg);
        throw std::runtime_error(msg);
    }
}

CollectionDescriptor CollectionDescriptor::Independent(
    ParameterType p, size_t initialCount
){
    return CollectionDescriptor(
        {p}, 
        CollectionStructure::INDEPENDENT,
        initialCount
    );
}

CollectionDescriptor CollectionDescriptor::Grouped(
    ParameterType p,
    size_t groupSize,
    size_t initialCount 
){
    if ( groupSize < 2 ){
        auto msg = "Grouped collections must have size > 1" ;
        SPDLOG_ERROR(msg);
        throw std::invalid_argument(msg);
    }

    return CollectionDescriptor(
        {p},
        CollectionStructure::GROUPED,
        initialCount,
        groupSize
    );
}

CollectionDescriptor CollectionDescriptor::Synchronized(
    std::vector<ParameterType> params,
    size_t initialCount
){
    if ( params.size() < 2 ){
        auto msg = "Synchronized collections require at least 2 parameter types" ;
        SPDLOG_ERROR(msg);
        throw std::invalid_argument(msg);
    }

    return CollectionDescriptor(
        std::move(params),
        CollectionStructure::SYNCHRONIZED,
        initialCount
    );
}

std::span<const ParameterType> CollectionDescriptor::parameters() const {
    return parameters_ ;
}

CollectionStructure CollectionDescriptor::structure() const {
    return structure_ ;
}

size_t CollectionDescriptor::groupSize() const {
    return groupSize_ ;
}

size_t CollectionDescriptor::initialCount() const {
    return initialCount_ ;
}

bool CollectionDescriptor::isValid() const {
    // outside of internal checks, 
    for ( const auto& p : parameters_ ){
        if ( initialCount_ >= p.supports().maxSlots ){
            return false ;
        }
    }

    return true ;
}

size_t CollectionHash::operator()(const CollectionDescriptor& collection) const {
    size_t seed = 0 ;

    // Fibonacci Hashing
    auto hashCombine = [&seed](size_t value) {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };

    hashCombine(std::hash<int>{}(
        static_cast<int>(collection.structure())
    ));

    for ( const auto& param : collection.parameters() ){
        hashCombine(std::hash<uint8_t>{}(param.to_uint8()));
    }

    if ( collection.structure() == CollectionStructure::GROUPED ){
        hashCombine(std::hash<size_t>{}(collection.groupSize()));
    }

    return seed ;
}

CollectionDescriptor nlohmann::adl_serializer<CollectionDescriptor>::from_json(const json& j){
    CollectionStructure st = j.at("structure");
    const auto& parameters = j.at("parameters");
    const auto& initialCount = j.at("initialCount");

    switch(st){
    case CollectionStructure::GROUPED:
        return CollectionDescriptor::Grouped(
            parameters[0], 
            j.at("groupSize"), 
            initialCount
        );
    case CollectionStructure::INDEPENDENT:
        return CollectionDescriptor::Independent(
            parameters[0],
            initialCount
        );
    case CollectionStructure::SYNCHRONIZED:
        return CollectionDescriptor::Synchronized(
            parameters,
            initialCount
        );
    default:
        throw std::invalid_argument("unrecognized collection structure");
    }
}

void nlohmann::adl_serializer<CollectionDescriptor>::to_json(json& j, const CollectionDescriptor& cd){
    j["structure"] = cd.structure() ;
    j["parameters"] = cd.parameters() ;
    
    if ( cd.structure() == CollectionStructure::GROUPED ){
        j["groupSize"] = cd.groupSize() ;
    }
    
    j["initialCount"] = cd.initialCount();
}