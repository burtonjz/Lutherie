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

#include "requests/ConnectionRequest.hpp"
#include <spdlog/spdlog.h>

ConnectionRequest::ConnectionRequest(
    const ConnectionEndpoint& outbound,
    const ConnectionEndpoint& inbound,
    bool remove,
    bool modulationDepthConnection
):
    outbound_(outbound),
    inbound_(inbound),
    remove_(remove)
{
    if ( !valid() ){
        throw std::runtime_error("Cannot create an invalid ConnectionRequest");
    }

    // depth should always be false unless it's a modulation inbound
    // (for comparison operations)
    depth_ = inbound.socket() == SocketType::ModulationInbound 
        ? modulationDepthConnection 
        : false ;
}

bool ConnectionRequest::operator==(const ConnectionRequest& other) const {
    return inbound_ == other.inbound_
        && outbound_ == other.outbound_
        && depth_ == other.depth_ ;
}

bool ConnectionRequest::operator<(const ConnectionRequest& other) const {
    if ( inbound_ != other.inbound_ ){
        return inbound_ < other.inbound_ ;
    }

    if ( outbound_ != other.outbound_ ){
        return outbound_ < other.outbound_ ;
    }

    return depth_ < other.depth_ ;
}

bool ConnectionRequest::valid() const {
    if ( !inbound_.socket().isInbound() ){
        SPDLOG_ERROR("Cannot set inbound endpoint to an object with an outbound socket");
        return false ;
    }

    if ( outbound_.socket().isInbound() ){
        SPDLOG_ERROR("Cannot set outbound endpoint to an object with an inbound socket");
        return false ;
    }

    if ( inbound_.socket().getMatchingType() != outbound_.socket() ){
        SPDLOG_ERROR(
            "Cannot set inbound socket {} to outbound socket {}",
            inbound_.socket().toString(), outbound_.socket().toString()
        );
    }
    return true ;
}

bool ConnectionRequest::remove() const {
    return remove_ ;
}

void ConnectionRequest::setRemove(bool remove){
    remove_ = remove ;
}

bool ConnectionRequest::modulatingDepth() const {
    return depth_ ;
}

void ConnectionRequest::setModulatingDepth(bool depth){
    if ( inbound_.socket() != SocketType::ModulationInbound ){
        SPDLOG_WARN("calling modulating depth on a non-modulating inbound socket is a no-op");
        return ;
    }
    depth_ = depth ;
}

const ConnectionEndpoint& ConnectionRequest::outbound() const {
    return outbound_ ;
}

const ConnectionEndpoint& ConnectionRequest::inbound() const {
    return inbound_ ;
}

bool ConnectionRequest::partialMatch(const ConnectionEndpoint& other) const {
    if ( other.socket().isInbound() ){
        return other == inbound_ ;
    } else {
        return other == outbound_ ;
    }
}

ConnectionRequest nlohmann::adl_serializer<ConnectionRequest>::from_json(const json& j){
    if ( !j.contains("inbound") || !j.contains("outbound") ){
        throw std::runtime_error("'inbound' and 'outbound' json objects must be defined.");
    }
    ConnectionEndpoint outbound = j.at("outbound");
    ConnectionEndpoint inbound = j.at("inbound");
    
    ConnectionRequest req(outbound, inbound);

    if ( !j.contains("action") || !j.at("action").is_string() ){
        throw std::runtime_error("'action' field must be present and appropriately defined.");
    }

    if ( j.at("action") == "create_connection" ){
        req.setRemove(false);
    } else if ( j.at("action") == "remove_connection" ){
        req.setRemove(true);
    } else if ( j.at("action") == "create_depth_connection" ){
        req.setRemove(false);
        req.setModulatingDepth(true);
    } else if ( j.at("action") == "remove_depth_connection" ){
        req.setRemove(true);
        req.setModulatingDepth(true);
    } else {
        throw std::runtime_error("invalid action specified for connection request.");
    }

    return req ;
}

void nlohmann::adl_serializer<ConnectionRequest>::to_json(json& j, const ConnectionRequest& req){
    j["outbound"] = req.outbound();
    j["inbound"] = req.inbound();

    bool remove = req.remove();
    bool depth = req.modulatingDepth();

    if ( depth ){
        if ( remove ){
            j["action"] = "remove_depth_connection" ;
        } else {
            j["action"] = "create_depth_connection" ;
        }
    } else {
        if ( remove ){
            j["action"] = "remove_connection" ;
        } else {
            j["action"] = "create_connection" ;
        }
    }
}