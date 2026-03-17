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

#include "graphics/PeripheralNode.hpp"

PeripheralNode::PeripheralNode(int deviceId, const QString& name, QGraphicsItem* parent):
    GraphNode(name,parent),
    deviceId_(deviceId)
{}

int PeripheralNode::getId() const {
    return deviceId_ ;
}

json PeripheralNode::serialize() const {
    json msg = GraphNode::serialize();
    msg["node_type"] = "PeripheralNode" ;
    msg["deviceId"] = deviceId_ ;

    return msg ;
}

void PeripheralNode::deserialize(const json& node){
    GraphNode::deserialize(node);
    if ( node.contains("deviceId") && node.at("deviceId").is_number() ){
        deviceId_ = node.at("deviceId");
    }
}