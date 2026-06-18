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

#ifndef CONNECTION_REQUEST_HPP_
#define CONNECTION_REQUEST_HPP_

#include "types/SocketType.hpp"
#include "types/ParameterType.hpp"
#include <optional>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json ;

struct ConnectionRequest {
    SocketType inboundSocket ;
    SocketType outboundSocket ;
    std::optional<int> inboundID ;
    std::optional<size_t> inboundIdx ; // audio only
    std::optional<int> outboundID ;
    std::optional<size_t> outboundIdx ; // audio only
    std::optional<ParameterType> inboundParameter ;
    bool depthConnection = false ;
    bool remove = false ;

    bool operator==(const ConnectionRequest& other) const {
        return inboundSocket == other.inboundSocket &&
               outboundSocket == other.outboundSocket &&
               inboundID == other.inboundID &&
               inboundIdx == other.inboundIdx &&
               outboundID == other.outboundID &&
               outboundIdx == other.outboundIdx &&
               inboundParameter == other.inboundParameter &&
               depthConnection == other.depthConnection ;
    }

    bool valid() const {
        bool t = true ;
        switch(inboundSocket){
        case SocketType::SignalInbound: 
            t = t && outboundSocket == SocketType::SignalOutbound ;
            // must have audio port indices defined
            t = t && inboundIdx.has_value();
            t = t && outboundIdx.has_value();
            break ;
        case SocketType::BufferInbound:
            t = t && outboundSocket == SocketType::BufferOutbound ;
            // must have audio port indices defined
            t = t && inboundIdx.has_value();
            t = t && outboundIdx.has_value();
            break ;
        case SocketType::MidiInbound:
            t = t && outboundSocket == SocketType::MidiOutbound ;
            break ;
        case SocketType::ModulationInbound:
            t = t && outboundSocket == SocketType::ModulationOutbound ;
            t = t && inboundParameter.has_value() ;
            break ;
        default:
            t = false ;
            break ;
        }

        return t ;
    }

    auto operator<=>(const ConnectionRequest&) const = default ;
};

inline void to_json(json& j, const ConnectionRequest& req){
    if ( req.depthConnection ){
        if ( req.remove ){
            j["action"] = "remove_depth_connection" ;
        } else {
            j["action"] = "create_depth_connection" ;
        }
    } else {
        if ( req.remove ){
            j["action"] = "remove_connection" ;
        } else {
            j["action"] = "create_connection" ;
        }
    }
    
    j["inbound"] = json::object();
    j["outbound"] = json::object();
    j["inbound"]["socketType"] = socketType2String(req.inboundSocket);
    j["outbound"]["socketType"] = socketType2String(req.outboundSocket);
    if ( req.inboundID.has_value() ) j["inbound"]["componentId"] = req.inboundID.value();
    if ( req.inboundIdx.has_value() ) j["inbound"]["index"] = req.inboundIdx.value();
    if ( req.outboundID.has_value() ) j["outbound"]["componentId"] = req.outboundID.value();
    if ( req.outboundIdx.has_value() ) j["outbound"]["index"] = req.outboundIdx.value();
    if ( req.inboundParameter.has_value() ) j["inbound"]["parameter"] = GET_PARAMETER_TRAIT_MEMBER(req.inboundParameter.value(), name);
}

inline void from_json(const json& j, ConnectionRequest& req){
    const auto& inbound = j.at("inbound");
    const auto& outbound = j.at("outbound");
    
    req.inboundSocket = socketTypeFromString(inbound.at("socketType"));
    req.outboundSocket = socketTypeFromString(outbound.at("socketType"));
    if ( inbound.contains("componentId") ) 
        req.inboundID = inbound.at("componentId");
    if ( inbound.contains("index") ) 
        req.inboundIdx = inbound.at("index");
    if ( outbound.contains("componentId") ) 
        req.outboundID = outbound.at("componentId");
    if ( outbound.contains("index") ) 
        req.outboundIdx = outbound.at("index");
    if ( inbound.contains("parameter") ) 
        req.inboundParameter = stringToParameter(inbound.at("parameter"));

    if ( j.at("action") == "create_connection" ){
        req.remove = false ;
    } else if ( j.at("action") == "remove_connection" ){
        req.remove = true ;
    } else if ( j.at("action") == "create_depth_connection" ){
        req.depthConnection = true ;
    } else if ( j.at("action") == "remove_depth_connection" ){
        req.remove = true ;
        req.depthConnection = true ;
    } else {
        throw std::runtime_error("invalid action specified for connection request.");
    }
}

#endif // CONNECTION_REQUEST_HPP_