/*
 * Copyright (C) 2025 Jared Burton
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

#include "managers/ConnectionManager.hpp"
#include "requests/ConnectionRequest.hpp"
#include "api/ApiClient.hpp"

#include <QGraphicsItem>
#include <QDebug>
#include <algorithm>

ConnectionManager::ConnectionManager(QObject* parent): 
    QObject(parent)
{
    connect(ApiClient::instance(), &ApiClient::dataReceived, this, &ConnectionManager::onApiDataReceived);
}

void ConnectionManager::loadConnection(const ConnectionRequest& req){
    if ( !req.valid() ){
        qWarning() << "Connection failed: Json is not a valid ConnectionRequest." ;
        return ;
    }

    if ( connectionExists(req) ){
        qWarning() << "Connection already exists. Will not load connection again.";
        return ;
    }
    
    // make sure sockets are real before adding the connection
    SocketWidget* inboundSocket = socketLookup_->findSocket({
        .type = req.inboundSocket, 
        .componentId = req.inboundID,
        .idx =  req.inboundIdx
    });

    SocketWidget* outboundSocket = socketLookup_->findSocket({
        .type = req.outboundSocket,
        .componentId = req.outboundID,
        .idx = req.outboundIdx,
    });

    if ( ! inboundSocket || ! outboundSocket ){
        qWarning() << "Json connection not successfully loaded: sockets not found";
        return ;
    }

    emit connectionAdded(req);
}

std::vector<ParameterType> ConnectionManager::getModulationConnections(int componentId) const {
    std::vector<ParameterType> sc ;

    for ( const auto& c : connections_ ){
        if ( 
            c.inboundSocket == SocketType::ModulationInbound && 
            c.inboundID == componentId &&
            c.inboundParameter.has_value() &&
            c.depthConnection == false 
        ){
            sc.push_back(c.inboundParameter.value());
        } 
    }

    return sc ;
}

std::vector<ParameterType> ConnectionManager::getModulationDepthConnections(int componentId) const {
    std::vector<ParameterType> sc ;

    for ( const auto& c : connections_ ){
        if ( 
            c.inboundSocket == SocketType::ModulationInbound && 
            c.inboundID == componentId &&
            c.inboundParameter.has_value() &&
            c.depthConnection == true 
        ){
            sc.push_back(c.inboundParameter.value());
        } 
    }

    return sc ;
}

void ConnectionManager::requestConnectionEvent(const ConnectionRequest& req){
    if ( !req.valid() ){
        qDebug() << nlohmann::json(req).dump() ;
        qWarning() << "Invalid connection request created. Cancelling connection.";
        return ;
    }

    sendConnectionApiRequest(req);    
}

void ConnectionManager::sendConnectionApiRequest(ConnectionRequest req){
    auto obj = req ;
    ApiClient::instance()->sendMessage(obj);
}

bool ConnectionManager::connectionExists(ConnectionRequest request) const {
    auto it = std::find_if(connections_.begin(), connections_.end(), [&request](const ConnectionRequest& r){
        return r == request ;
    });

    return it != connections_.end() ;
}

void ConnectionManager::onApiDataReceived(const json& msg){
    QString action = QString::fromStdString(msg.at("action")) ;
    bool success = msg.at("status") == "success" ;

    // handle any kind of connection request. Any other api actions should be handled above this one
    if ( ! success ) return ;

    ConnectionRequest req ;
    try {
        req = msg ;
    } catch (std::exception& e){
        return ;
    }

    if ( req.remove ){
        if ( !connectionExists(req) ){
            qWarning() << "requested connection is not present in connection manager. will not trigger removal.";
            return ;
        }
        connections_.erase(std::remove(
            connections_.begin(), connections_.end(), req),
            connections_.end()
        );
        emit connectionRemoved(req);
    } else {
        if ( connectionExists(req) ){
            qWarning() << "requested connection already exists in connection manager. Will not add again.";
            return ;
        }
        connections_.push_back(req);
        emit connectionAdded(req);
    }
}