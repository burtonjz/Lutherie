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

#ifndef CONNECTION_ENDPOINT_HPP_
#define CONNECTION_ENDPOINT_HPP_

#include "types/NodeType.hpp"
#include "types/SocketType.hpp"
#include "types/ParameterType.hpp"

#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;


class ConnectionEndpoint {
private:
    NodeType type_ ;
    SocketType socket_ ;
    int id_ ; 

    std::optional<size_t> index_ ; 
    std::optional<ParameterType> param_ ; 
    
public:
    static ConnectionEndpoint create(
        NodeType type_,
        SocketType socket,
        int id,
        std::optional<size_t> index = std::nullopt,
        std::optional<ParameterType> modulatedParam = std::nullopt 
    );

    auto operator<=>(const ConnectionEndpoint& ) const = default ;

    NodeType type() const ;
    SocketType socket() const ;
    
    const std::optional<size_t>& index() const ;
    int id() const ;
    const std::optional<ParameterType>& modulatedParam() const ;
    
    std::string toString() const ;

private:
    ConnectionEndpoint(
        NodeType type,
        SocketType socket,
        int id,
        std::optional<size_t> index = std::nullopt,
        std::optional<ParameterType> modulatedParam = std::nullopt 
    );

    static ConnectionEndpoint standard(NodeType type, SocketType socket, int id);
    static ConnectionEndpoint audio(NodeType type, SocketType socket, size_t index, int id);
    static ConnectionEndpoint modulationIn(NodeType type, SocketType socket, int id, ParameterType param);
    
};

struct EndpointHash {
    size_t operator()(const ConnectionEndpoint& endpoint) const ;
};

// JSON (de)serialization
namespace nlohmann {
template <>
struct adl_serializer<ConnectionEndpoint> {
    static ConnectionEndpoint from_json(const json& j);
    static void to_json(json& j, const ConnectionEndpoint& endpoint);
};
} 


#endif // CONNECTION_ENDPOINT_HPP_