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

#include "types/SocketType.hpp"
#include "types/ParameterType.hpp"

#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class ConnectionEndpoint {
private:
    SocketType socket_ ;
    std::optional<size_t> index_ ; 
    std::optional<int> id_ ; 
    std::optional<ParameterType> param_ ; 
    
public:
    static ConnectionEndpoint create(
        SocketType socket,
        std::optional<size_t> index = std::nullopt,
        std::optional<int> componentId = std::nullopt,
        std::optional<ParameterType> modulatedParam = std::nullopt 
    );

    auto operator<=>(const ConnectionEndpoint& ) const = default ;

    SocketType socket() const ;
    
    const std::optional<size_t>& index() const ;
    const std::optional<int>& componentId() const ;
    const std::optional<ParameterType>& modulatedParam() const ;

private:
    ConnectionEndpoint(
        SocketType socket,
        std::optional<size_t> index = std::nullopt,
        std::optional<int> componentId = std::nullopt,
        std::optional<ParameterType> modulatedParam = std::nullopt 
    );

    static ConnectionEndpoint standard(SocketType socket, std::optional<int> componentId);
    static ConnectionEndpoint audio(SocketType socket, size_t index, std::optional<int> componentId);
    static ConnectionEndpoint modulationIn(SocketType socket, int componentId, ParameterType param);
    
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