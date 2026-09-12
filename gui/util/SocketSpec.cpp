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

#include "util/SocketSpec.hpp"

#include <spdlog/spdlog.h>

SocketSpec::SocketSpec(const QString& name, ConnectionEndpoint endpoint):
    name_(name),
    data_(std::move(endpoint))
{}

SocketSpec::SocketSpec(const QString& name, std::vector<ConnectionEndpoint> points):
    name_(name),
    data_(std::move(points))
{
    if ( endpoints().size() < 2 ){
        throw std::runtime_error("Cannot create a Group SocketSpec with less than 2 members.");
    }

    if ( !valid() ){
        throw std::runtime_error("Cannot create an invalid Group SocketSpec.");
    }
}

SocketType SocketSpec::type() const {
    return endpoints()[0].socket() ;
}

const QString& SocketSpec::name() const {
    return name_ ;
}

void SocketSpec::setName(const QString& name){
    name_ = name ;
}

bool SocketSpec::isGroup() const {
    return std::holds_alternative<EndpointGroup>(data_);
}

std::span<const ConnectionEndpoint> SocketSpec::endpoints() const {
    if (auto* single = std::get_if<ConnectionEndpoint>(&data_))
        return {single, 1};
    return std::get<EndpointGroup>(data_);
}

std::set<int> SocketSpec::componentIds() const {
    std::set<int> v ;
    for ( const auto& endpoint : endpoints() ){
        if ( endpoint.componentId().has_value () ){
            v.insert(endpoint.componentId().value());
        }
    }
    return v ;
}

bool SocketSpec::valid() const {
    if ( std::holds_alternative<ConnectionEndpoint>(data_) ) return true ;

    if ( endpoints().size() < 2 ){
        SPDLOG_ERROR("A Grouped SocketSpec cannot have less than 2 members.");
        return false ;
    }

    const auto& componentId = endpoints()[0].componentId() ;
    for ( const auto& endpoint : endpoints() ){
        if ( endpoint.socket() != type() ){
            SPDLOG_ERROR(
                "Cannot create a Grouped SocketSpec where endpoints have differing types"
            );
            return false ;
        }
        if ( type() == SocketType::ModulationInbound ){
            if ( endpoint.componentId() != componentId ){
                SPDLOG_ERROR("cannot create a group modulation inbound socket where all component ids do not match");
                return false ;
            }
        } 
    }

    if ( type() == SocketType::ModulationOutbound ){
        SPDLOG_ERROR("Cannot create a group outbound modulation socket.");
        return false ;
    }

    return true ;
}

bool SocketSpec::includes(const ConnectionEndpoint& endpoint) const {
    for ( const auto& e : endpoints() ){
        if ( endpoint == e ) return true ;
    }
    return false ;
}

void SocketSpec::add(const ConnectionEndpoint& endpoint){
    if ( !isGroup() ){
        throw std::runtime_error("This is a single endpoint socket spec and doesn't support grouping");
    }

    auto& points = std::get<EndpointGroup>(data_);
    auto it = std::find(points.begin(), points.end(), endpoint);
    if ( it != points.end() ){
        SPDLOG_WARN("spec is already a member of this group. ignoring add.");
        return ;
    }

    points.push_back(endpoint);
}

SocketSpec::RemoveResult SocketSpec::remove(const ConnectionEndpoint& endpoint){
    if ( !isGroup() ){
        throw std::runtime_error("cannot remove SocketSpec when this is not a group spec.");
    }

    auto& points = std::get<EndpointGroup>(data_);

    auto it = std::find(points.begin(), points.end(), endpoint);
    if ( it == points.end() ){
        return NotFound ;
    }

    if ( points.size() <= 2 ){
        return RequiresDispersal ;
    }

    points.erase(it);
    return Removed ;
}

// JSON (de)serialization

SocketSpec nlohmann::adl_serializer<SocketSpec>::from_json(const json& j){
    if ( !j.contains("name") || !j.at("name").is_string() ){
        throw std::runtime_error("cannot deserialize SocketSpec: invalid or missing 'name'.");
    }
    QString name = QString::fromStdString(j.at("name").get<std::string>());

    if ( !j.contains("isGroup") || !j.at("isGroup").is_boolean() ){
        throw std::runtime_error("cannot deserialize SocketSpec: invalid or missing 'isGroup'.");
    }
    
    if ( j.at("isGroup").get<bool>() ){
        if ( !j.contains("endpoints") || !j.at("endpoints").is_array() ){
            throw std::runtime_error("'endpoints' are not defined. Cannot deserialize.");
        }
        EndpointGroup endpoints = j.at("endpoints");
        return SocketSpec(name, endpoints);
    } else {
        if ( !j.contains("endpoint") ){
            throw std::runtime_error("'endpoint' is not defined. cannot deserialize.");
        }
        ConnectionEndpoint endpoint = j.at("endpoint");
        return SocketSpec(name, endpoint);
    }
}

void nlohmann::adl_serializer<SocketSpec>::to_json(json& j, const SocketSpec& spec){
    j["name"] = spec.name().toStdString();
    j["isGroup"] = spec.isGroup();

    if ( spec.isGroup() ){
        j["isGroup"] = true ;
        j["endpoints"] = spec.endpoints();
    } else {
        j["isGroup"] = false ;
        j["endpoint"] = spec.endpoints()[0];
    }
}