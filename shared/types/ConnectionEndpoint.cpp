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
    SocketType socket,
    std::optional<size_t> index,
    std::optional<int> componentId,
    std::optional<ParameterType> modulatedParam 
):
    socket_(socket),
    index_(index),
    id_(componentId),
    param_(modulatedParam)
{}

ConnectionEndpoint ConnectionEndpoint::create(
        SocketType socket,
        std::optional<size_t> index,
        std::optional<int> componentId,
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
        return audio(socket, index.value(),componentId);
    } 
    
    if ( socket == SocketType::ModulationInbound ){
        if ( !componentId.has_value() || !modulatedParam.has_value() ){
            std::string err = fmt::format(
                "cannot create ConnectionEndpoint with SocketType {}"
                " without a valid component id and modulated parameter.",
                socket.toString()
            );
            SPDLOG_ERROR(err);
            throw std::runtime_error(err);
        }
        return modulationIn(socket, componentId.value(), modulatedParam.value());
    } 

    return standard(socket, componentId);
}

ConnectionEndpoint ConnectionEndpoint::standard(SocketType socket, std::optional<int> componentId){
    return ConnectionEndpoint(socket, std::nullopt, componentId, std::nullopt);
}

ConnectionEndpoint ConnectionEndpoint::audio(SocketType socket, size_t index, std::optional<int> componentId){
    return ConnectionEndpoint(socket, index, componentId, std::nullopt);
}

ConnectionEndpoint ConnectionEndpoint::modulationIn(SocketType socket, int componentId, ParameterType param){
    return ConnectionEndpoint(socket, std::nullopt, componentId, param);
}

SocketType ConnectionEndpoint::socket() const {
    return socket_ ;
}   

const std::optional<size_t>& ConnectionEndpoint::index() const {
    return index_ ;
}

const std::optional<int>& ConnectionEndpoint::componentId() const {
    return id_ ;
}

const std::optional<ParameterType>& ConnectionEndpoint::modulatedParam() const {
    return param_ ;
}

ConnectionEndpoint nlohmann::adl_serializer<ConnectionEndpoint>::from_json(const json& j){
    if ( !j.contains("socket") ){
        throw std::runtime_error("socket not present in json");
    }
    SocketType socket = j.at("socket");

    std::optional<size_t> index = std::nullopt ;
    if ( j.contains("index") && j.at("index").is_number_integer() ){
        index = j.at("index");
    }

    std::optional<int> componentId = std::nullopt ;
    if ( j.contains("componentId") && j.at("componentId").is_number_integer() ){
        componentId = j.at("componentId");
    }

    std::optional<ParameterType> param = std::nullopt ;
    if ( j.contains("parameter") && j.at("parameter").is_string() ){
        ParameterType p = stringToParameter(j.at("parameter"));
        param = p ;
    }

    return ConnectionEndpoint::create(socket, index, componentId, param);
}

void nlohmann::adl_serializer<ConnectionEndpoint>::to_json(json& j, const ConnectionEndpoint& endpoint){
    j["socket"] = endpoint.socket();

    const auto& index = endpoint.index();
    if ( index.has_value() ) j["index"] = index.value();

    const auto& id = endpoint.componentId();
    if ( id.has_value() ) j["componentId"] = id.value();

    const auto& param = endpoint.modulatedParam();
    if ( param.has_value() ) j["parameter"] = GET_PARAMETER_TRAIT_MEMBER(param.value(), name);
}