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

#ifndef __SHARED_COMPONENT_DESCRIPTOR_HPP_
#define __SHARED_COMPONENT_DESCRIPTOR_HPP_

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "types/CollectionType.hpp"
#include "types/ParameterType.hpp"
#include "types/ComponentType.hpp"
#include "meta/CollectionDescriptor.hpp"

struct ComponentDescriptor {
    std::string name ;
    ComponentType type ;
    std::vector<ParameterType> modulatableParameters ;
    std::vector<ParameterType> controllableParameters ;
    std::vector<CollectionDescriptor> collections ;

    size_t numAudioInputs  ;
    size_t numAudioOutputs ;
    size_t numMidiInputs ;
    size_t numMidiOutputs ;

    bool canModulate ;

    bool isModule() const { return numAudioOutputs > 0 ; }
    bool isModulator() const { return canModulate ; }
    bool isMidiHandler() const { return numMidiOutputs > 0 ; }
    bool isMidiListener() const { return numMidiInputs > 0 ; }
    bool isAnalyzer() const { return numAudioInputs > 0 && numAudioOutputs == 0 ;}

    int hasCollection(CollectionType c) const {
        for ( size_t i = 0 ; i < collections.size() ; ++i ){
            if ( collections[i].collectionType == c ){
                return i ;
            }
        }
        return -1 ;
    }

    const CollectionDescriptor& getCollection(size_t i) const {
        return collections[i] ;
    }

    const CollectionDescriptor& getCollection(CollectionType c) const {
        int i = hasCollection(c);
        if ( i == -1 ){
            std::string msg = fmt::format("No collection found with name {}", name);
            SPDLOG_ERROR(msg);
            throw std::runtime_error(msg);
        }

        return getCollection(i);
    }
};

#endif // __SHARED_COMPONENT_DESCRIPTOR_HPP_