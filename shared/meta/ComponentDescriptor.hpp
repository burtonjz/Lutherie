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

#include "types/ParameterType.hpp"
#include "types/ComponentType.hpp"
#include "meta/CollectionDescriptor.hpp"

struct ComponentDescriptor {
    std::string name ;
    ComponentType type ;
    std::vector<ParameterType> modulatableParameters = {};
    std::vector<ParameterType> controllableParameters = {};
    std::optional<CollectionDescriptor> collection = std::nullopt ;

    size_t numSignalInputs  = 0 ;
    size_t numSignalOutputs = 0 ;
    size_t numBufferInputs  = 0 ;
    size_t numBufferOutputs = 0 ;
    size_t numMidiInputs    = 0 ;
    size_t numMidiOutputs   = 0 ;
    
    bool canModulate = false ;
    bool hasFile = false ;
    bool allowMultipleBufferConnections = true ;

    bool isSignalComponent() const { return numSignalInputs > 0 || numSignalOutputs > 0 ; }
    bool isBufferComponent() const { return numBufferInputs > 0 || numBufferOutputs > 0 ; }
    bool isModulator() const { return canModulate ; }
    bool isMidiHandler() const { return numMidiOutputs > 0 ; }
    bool isMidiListener() const { return numMidiInputs > 0 ; }
    bool isAnalyzer() const { 
        return numSignalInputs > 0 && numSignalOutputs == 0 
            && numBufferInputs == 0 && numBufferOutputs == 0 ;
    }

    bool hasCollection() const {
        return collection.has_value() ;
    }

    const CollectionDescriptor& getCollection() const {
        if ( !hasCollection() ){
            std::string msg = fmt::format("No collection found for component ", name);
            SPDLOG_ERROR(msg);
            throw std::runtime_error(msg);
        }
        return collection.value() ;
    }

    bool shouldShowControlParameters() const {
        return controllableParameters.size() > 0 || hasFile || hasCollection() ;
    }

    bool shouldShowModulationParameters() const {
        return modulatableParameters.size() > 0 ;
    }
};

#endif // __SHARED_COMPONENT_DESCRIPTOR_HPP_