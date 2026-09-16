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
#include "util/SocketNamer.hpp"

#include <spdlog/spdlog.h>

SocketSpec::SocketSpec(ConnectionEndpoint endpoint):
    data_({endpoint})
{
    if ( !valid() ){
        throw std::runtime_error("Cannot create an invalid Group SocketSpec.");
    }

    updateName();
}

SocketSpec::SocketSpec(std::vector<ConnectionEndpoint> points):
    data_(std::move(points))
{
    if ( !valid() ){
        throw std::runtime_error("Cannot create an invalid Group SocketSpec.");
    }

    updateName();
}

SocketType SocketSpec::type() const {
    return endpoints()[0].socket() ;
}

size_t SocketSpec::numEndpoints() const {
    return data_.size();
}

std::span<const ConnectionEndpoint> SocketSpec::endpoints() const {
    return data_ ;
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
    if ( numEndpoints() == 1 ) return true ;

    if ( endpoints().size() < 2 ){
        SPDLOG_ERROR("A Grouped SocketSpec cannot have less than 2 members.");
        return false ;
    }

    // const auto& componentId = endpoints()[0].componentId() ;
    for ( const auto& endpoint : endpoints() ){
        if ( endpoint.socket() != type() ){
            SPDLOG_ERROR(
                "Cannot create a Grouped SocketSpec where endpoints have differing types"
            );
            return false ;
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
    auto e = endpoints();
    auto it = std::find(e.begin(), e.end(), endpoint);

    if ( it != e.end() ){
        SPDLOG_WARN("spec is already a member of this group. ignoring add.");
        return ;
    }
    data_.push_back(endpoint);
    updateName();
}

bool SocketSpec::remove(const ConnectionEndpoint& endpoint){
    if ( numEndpoints() == 1 ){
        throw std::runtime_error("cannot remove SocketSpec when this is not a group spec.");
    }

    auto it = std::find(data_.begin(), data_.end(), endpoint);
    if ( it == data_.end() ) return false ;
        
    data_.erase(it);
    updateName();
    
    return true ;
}

bool SocketSpec::mergeWith(const SocketSpec& other){
    if ( other.type() != type() ){
        SPDLOG_WARN("Cannot merge SocketSpec with differing type");
        return false ;
    }


    for ( const auto& endpoint : other.endpoints() ){
        if ( includes(endpoint) ) continue ;
        add(endpoint);
    }

    return true ;
}

QString SocketSpec::name() const {
    return name_ ;
}

void SocketSpec::updateName(){
    name_ = QString::fromStdString(SocketNamer::compute(*this));
}

std::string SocketSpec::toString() const {
    json j = *this ;
    return j.dump();
}

// JSON (de)serialization

SocketSpec nlohmann::adl_serializer<SocketSpec>::from_json(const json& j){
    if ( !j.contains("endpoints") || !j.at("endpoints").is_array() ){
        throw std::runtime_error("'endpoints' are not defined. Cannot deserialize.");
    }

    std::vector<ConnectionEndpoint> endpoints = j.at("endpoints");    
    return SocketSpec(endpoints);
}

void nlohmann::adl_serializer<SocketSpec>::to_json(json& j, const SocketSpec& spec){
    j["endpoints"] = spec.endpoints();
}