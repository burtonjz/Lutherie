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

#ifndef SOCKET_SPEC_HPP_
#define SOCKET_SPEC_HPP_

#include <vector>
#include <set>
#include <nlohmann/json.hpp>

#include <QString>

#include "types/ConnectionEndpoint.hpp"
#include "types/SocketType.hpp"

using json = nlohmann::json ;

class SocketSpec {
private:
    std::vector<ConnectionEndpoint> data_ ;
    QString name_ ;

public:
    SocketSpec(ConnectionEndpoint endpoint);
    SocketSpec(std::vector<ConnectionEndpoint> points);

    bool operator<=>(const SocketSpec& other) const = default ;

    SocketType type() const ;
    
    size_t numEndpoints() const ;
    std::span<const ConnectionEndpoint> endpoints() const ;

    std::set<int> componentIds() const ;

    bool valid() const ;

    bool includes(const ConnectionEndpoint& endpoint) const ;

    void add(const ConnectionEndpoint& endpoint);
    bool remove(const ConnectionEndpoint& endpoint);

    bool mergeWith(const SocketSpec& other);

    QString name() const ;
    void updateName();

    std::string toString() const ;

};

// JSON (de)serialization
namespace nlohmann {
template <>
struct adl_serializer<SocketSpec> {
    static SocketSpec from_json(const json& j);
    static void to_json(json& j, const SocketSpec& spec);
};
} 

#endif // SOCKET_SPEC_HPP_