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

#include "types/ConnectionEndpoint.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class ConnectionRequest {
private:
    ConnectionEndpoint outbound_ ;
    ConnectionEndpoint inbound_ ;
    bool depth_ = false ;
    bool remove_ = false ;

public:
    ConnectionRequest(
        const ConnectionEndpoint& outbound,
        const ConnectionEndpoint& inbound,
        bool remove = false,
        bool modulationDepthConnection = false
    );

    bool operator==(const ConnectionRequest& other) const ;
    bool operator<(const ConnectionRequest& other) const ;

    bool valid() const ;

    bool remove() const ;
    void setRemove(bool remove);

    bool modulatingDepth() const ;
    void setModulatingDepth(bool depth);

    const ConnectionEndpoint& outbound() const ;
    const ConnectionEndpoint& inbound() const ;
    
    // if the endpoint matches either inbound or outbound
    bool partialMatch(const ConnectionEndpoint& other) const ;
    
};

// JSON (de)serialization
namespace nlohmann {
template <>
struct adl_serializer<ConnectionRequest> {
    static ConnectionRequest from_json(const json& j);
    static void to_json(json& j, const ConnectionRequest& req);
};
} 

#endif // CONNECTION_REQUEST_HPP_