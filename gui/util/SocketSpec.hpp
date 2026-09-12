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

#include <variant>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>

#include <QString>

#include "types/ConnectionEndpoint.hpp"
#include "types/SocketType.hpp"

using json = nlohmann::json ;
using EndpointGroup = std::vector<ConnectionEndpoint>;

class SocketSpec {
private:
    QString name_ = "" ;
    std::variant<ConnectionEndpoint,EndpointGroup> data_ ;

public:
    enum RemoveResult { Removed, RequiresDispersal, NotFound };

    SocketSpec(const QString& name, ConnectionEndpoint endpoint);
    SocketSpec(const QString& name, std::vector<ConnectionEndpoint> points);

    bool operator<=>(const SocketSpec& other) const = default ;

    SocketType type() const ;

    const QString& name() const ;
    void setName(const QString& name);

    bool isGroup() const ;
    std::span<const ConnectionEndpoint> endpoints() const ;

    std::set<int> componentIds() const ;

    bool valid() const ;

    bool includes(const ConnectionEndpoint& endpoint) const ;

    void add(const ConnectionEndpoint& endpoint);
    RemoveResult remove(const ConnectionEndpoint& endpoint);
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