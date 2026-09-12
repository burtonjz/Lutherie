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

#ifndef I_SOCKET_LOOKUP_HPP_
#define I_SOCKET_LOOKUP_HPP_

#include "graphics/SocketWidget.hpp"
#include <QPointF>

struct ModulationParameter {
    std::optional<ConnectionEndpoint> endpoint = std::nullopt ;
    bool depth = false ;
};

class ISocketLookup {
public:
    virtual ~ISocketLookup() = default ;

    virtual SocketWidget* findVisibleSocket(const SocketSpec& spec) const = 0 ;
    virtual SocketWidget* findVisibleSocket(const ConnectionEndpoint& endpoint) const = 0 ;

    /**
     * @brief find a socket widget at the specified location
     * 
     * @param scenePos 
     * @return SocketWidget* 
     */
    virtual SocketWidget* findSocketAt(const QPointF& scenePos) const = 0 ;

    virtual ModulationParameter requestModulationParameter(SocketWidget* socket) = 0 ;

};

#endif // I_SOCKET_LOOKUP_HPP_
