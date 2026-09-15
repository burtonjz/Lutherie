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

#include "requests/CollectionRequest.hpp"


const std::string CollectionRequest::actionToJson() const {
    for ( const auto& [a, s] : actionMap ){
        if ( action == a ) return s ;
    }
    throw std::runtime_error("Invalid action");
}   

CollectionAction CollectionRequest::actionFromJson(const std::string& str){
    for ( const auto& [a, s] : actionMap ){
        if ( str == s ) return a ;
    }
    throw std::runtime_error("Invalid action string");
}

bool CollectionRequest::valid(const CollectionDescriptor& d) const {
    bool isValid = true ;
    // make sure index is specified if expected
    if ( 
            action == CollectionAction::REMOVE || 
            action == CollectionAction::GET ||
            action == CollectionAction::SET
    ){
        isValid = isValid && index.has_value() ;
    }

    // now, depending on what collection structure is used,
    // check the value for validity
    if ( 
        action == CollectionAction::ADD ||
        action == CollectionAction::SET ||
        action == CollectionAction::ADD_ALL
    ){
        switch(d.structure){
        case CollectionStructure::INDEPENDENT:
            isValid = isValid && value.has_value() &&
                value->is_number() ;
            break ;
        case CollectionStructure::GROUPED:    
            isValid = isValid && value.has_value() &&
                value->is_array() && 
                value->size() == d.groupSize ;
            break ;
        case CollectionStructure::SYNCHRONIZED:
            isValid = isValid && value.has_value() &&
                value->is_object() &&
                validateSyncParams(d, *value);
            break ;
        }
    }

    return isValid ;
}

bool CollectionRequest::validateSyncParams(const CollectionDescriptor& d, const json& value) const {
    for ( auto p : d.params ){
        std::string pName = std::string(GET_PARAMETER_TRAIT_MEMBER(p, name));
        if ( !value.contains(pName) ) return false ;
        if ( !value[pName].is_number() ) return false ;
    }
    return true ;
}

std::string CollectionRequest::toString() const {
    json j = *this ;
    return j ;
}
