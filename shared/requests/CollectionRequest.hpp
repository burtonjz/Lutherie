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

#ifndef __COLLECTION_REQUEST_HPP_
#define __COLLECTION_REQUEST_HPP_

#include "meta/CollectionDescriptor.hpp"
#include "types/ParameterType.hpp"

#include <optional>
#include <nlohmann/json.hpp>


using json = nlohmann::json ;

enum class CollectionAction {
    ADD,
    REMOVE,
    GET,
    GET_ALL,
    SET,
    ADD_ALL,
    RESET
};

struct CollectionRequest {
    CollectionAction action ;
    int componentId ;

    std::optional<json> value ; // for ADD/SET
    std::optional<size_t> index ; // for REMOVE, output for ADD

    static constexpr std::array<std::pair<CollectionAction, const char*>, 7> actionMap = {{
        {CollectionAction::ADD, "add_collection_value"},
        {CollectionAction::REMOVE, "remove_collection_value"},
        {CollectionAction::GET, "get_collection_value"},
        {CollectionAction::GET_ALL, "get_collection_values"},
        {CollectionAction::SET, "set_collection_value"},
        {CollectionAction::ADD_ALL, "add_collection_values"},
        {CollectionAction::RESET, "reset_collection"}
    }};

    const std::string actionToJson() const {
        for ( const auto& [a, s] : actionMap ){
            if ( action == a ) return s ;
        }
        throw std::runtime_error("Invalid action");
    }   

    static CollectionAction actionFromJson(const std::string& str){
        for ( const auto& [a, s] : actionMap ){
            if ( str == s ) return a ;
        }
        throw std::runtime_error("Invalid action string");
    }

    bool valid(const CollectionDescriptor& d) const {
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

    bool validateSyncParams(const CollectionDescriptor& d, const json& value) const {
        for ( auto p : d.params ){
            std::string pName = std::string(GET_PARAMETER_TRAIT_MEMBER(p, name));
            if ( !value.contains(pName) ) return false ;
            if ( !value[pName].is_number() ) return false ;
        }
        return true ;
    }

};

inline void to_json(json& j, const CollectionRequest& req){
    j["action"] = req.actionToJson();
    j["componentId"] = req.componentId ;
    if ( req.value.has_value() ) j["value"] = *req.value ;
    if ( req.index.has_value() ) j["index"] = *req.index ;
}

inline void from_json(const json& j, CollectionRequest& req){
    req.action = req.actionFromJson(j["action"]);
    req.componentId = j["componentId"];
    if ( j.contains("value") ) req.value = j["value"] ;
    if ( j.contains("index") ) req.index = j["index"] ;
}

#endif // __COLLECTION_REQUEST_HPP_