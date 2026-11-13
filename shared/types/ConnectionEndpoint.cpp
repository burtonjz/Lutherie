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

#include "types/ConnectionEndpoint.hpp"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

ConnectionEndpoint::ConnectionEndpoint(
    NodeType type,
    SocketType socket,
    int id,
    std::optional<size_t> index,
    std::optional<ParameterType> modulatedParam 
):
    type_(type),
    socket_(socket),
    id_(id),
    index_(index),
    param_(modulatedParam)
{}

ConnectionEndpoint ConnectionEndpoint::create(
        NodeType type,
        SocketType socket,
        int id,
        std::optional<size_t> index,
        std::optional<ParameterType> modulatedParam 
){
    if ( socket.isAudio() ){
        if ( !index.has_value() ){
            std::string err = fmt::format(
                "cannot create ConnectionEndpoint with SocketType {}"
                " without a valid index.",
                socket.toString()
            );
            SPDLOG_ERROR(err);
            throw std::runtime_error(err);
        }
        return audio(type, socket, index.value(),id);
    } 
    
    if ( socket == SocketType::ModulationInbound ){
        if ( !modulatedParam.has_value() ){
            std::string err = fmt::format(
                "cannot create ConnectionEndpoint with SocketType {}"
                " without a valid component id and modulated parameter.",
                socket.toString()
            );
            SPDLOG_ERROR(err);
            throw std::runtime_error(err);
        }
        return modulationIn(type, socket, id, modulatedParam.value());
    } 

    return standard(type, socket, id);
}

ConnectionEndpoint ConnectionEndpoint::standard(NodeType type, SocketType socket, int id){
    return ConnectionEndpoint(type, socket, id, std::nullopt, std::nullopt);
}

ConnectionEndpoint ConnectionEndpoint::audio(NodeType type, SocketType socket, size_t index, int id){
    return ConnectionEndpoint(type, socket, id, index, std::nullopt);
}

ConnectionEndpoint ConnectionEndpoint::modulationIn(NodeType type, SocketType socket, int id, ParameterType param){
    return ConnectionEndpoint(type, socket, id, std::nullopt, param);
}

NodeType ConnectionEndpoint::type() const {
    return type_ ;
}

SocketType ConnectionEndpoint::socket() const {
    return socket_ ;
}   

const std::optional<size_t>& ConnectionEndpoint::index() const {
    return index_ ;
}

int ConnectionEndpoint::id() const {
    return id_ ;
}

const std::optional<ParameterType>& ConnectionEndpoint::modulatedParam() const {
    return param_ ;
}

std::string ConnectionEndpoint::toString() const {
    json j = *this ;
    return j.dump();
}

size_t EndpointHash::operator()(const ConnectionEndpoint& endpoint) const {
    size_t h = std::hash<NodeType::Value>()(endpoint.type());
    h ^= std::hash<SocketType::Value>()(endpoint.socket()) << 1 ;
    h ^= std::hash<int>()(endpoint.id()) << 1 ;

    const auto& index = endpoint.index();
    h ^= std::hash<bool>()(index.has_value()) << 1 ;
    if ( index ) h ^= std::hash<size_t>()(*index) << 1 ;

    const auto& param = endpoint.modulatedParam();
    h ^= std::hash<bool>()(param.has_value()) << 1 ;
    if ( param ) h ^= std::hash<ParameterType>()(*param) << 1 ;
    return h ;
}

ConnectionEndpoint nlohmann::adl_serializer<ConnectionEndpoint>::from_json(const json& j){
    if ( !j.contains("type") ){
        throw std::runtime_error("'type' not present in json");
    }
    NodeType type = j.at("type");

    if ( !j.contains("socket") ){
        throw std::runtime_error("'socket' not present in json");
    }
    SocketType socket = j.at("socket");

    if ( !j.contains("id") ){
        throw std::runtime_error("'id' not present in json");
    }
    int id = j.at("id");
    
    std::optional<size_t> index = std::nullopt ;
    if ( j.contains("index") && j.at("index").is_number_integer() ){
        index = j.at("index");
    }

    std::optional<ParameterType> param = std::nullopt ;
    if ( j.contains("parameter") && j.at("parameter").is_string() ){
        ParameterType p = stringToParameter(j.at("parameter"));
        param = p ;
    }

    return ConnectionEndpoint::create(type, socket, id, index, param);
}

void nlohmann::adl_serializer<ConnectionEndpoint>::to_json(json& j, const ConnectionEndpoint& endpoint){
    j["type"] = endpoint.type();
    j["socket"] = endpoint.socket();
    j["id"] = endpoint.id();

    const auto& index = endpoint.index();
    if ( index.has_value() ) j["index"] = index.value();

    const auto& param = endpoint.modulatedParam();
    if ( param.has_value() ) j["parameter"] = GET_PARAMETER_TRAIT_MEMBER(param.value(), name);
}