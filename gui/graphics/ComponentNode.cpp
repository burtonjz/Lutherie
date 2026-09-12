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

#include "graphics/ComponentNode.hpp"
#include "graphics/SocketWidget.hpp"
#include "types/ConnectionEndpoint.hpp"

#include <QGraphicsSceneMouseEvent>
#include <vector>

ComponentNode::ComponentNode(ComponentModel* model, QGraphicsItem* parent): 
    GraphNode(model->getName(), parent),
    model_(model)
{
    auto d = model_->getDescriptor();
    
    // create sockets from descriptor
    std::vector<SocketSpec> specs ;

    if ( d.modulatableParameters.size() > 0 ){
        std::vector<ConnectionEndpoint> endpoints ;
        for ( const auto& p : d.modulatableParameters ){
            endpoints.push_back(ConnectionEndpoint::create(
                SocketType::ModulationInbound, std::nullopt, 
                model_->getId(), p
            ));     
        }
        
        SocketSpec spec("Modulation Inputs", endpoints);
        specs.push_back(spec);
    }
    
    for ( size_t i = 0; i < d.numSignalInputs; ++i ){
        ConnectionEndpoint e = ConnectionEndpoint::create(
            SocketType::SignalInbound, i, model_->getId()
        );     
        SocketSpec spec(QString("Audio Input %1").arg(i+1), e);
        specs.push_back(spec);
    }

    for ( size_t i = 0; i < d.numBufferInputs; ++i ){
        ConnectionEndpoint e = ConnectionEndpoint::create(
            SocketType::BufferInbound, i, model_->getId()
        );     
        SocketSpec spec(QString("Buffer Input %1").arg(i+1), e);
        specs.push_back(spec);
    }

    for ( size_t i = 0; i < d.numMidiInputs; ++i ){
        ConnectionEndpoint e = ConnectionEndpoint::create(
            SocketType::MidiInbound, std::nullopt, model_->getId()
        );     
        SocketSpec spec(QString("MIDI Input %1").arg(i+1), e);
        specs.push_back(spec);
    }

    for ( size_t i = 0; i < d.numSignalOutputs; ++i ){
        ConnectionEndpoint e = ConnectionEndpoint::create(
            SocketType::SignalOutbound, i, model_->getId()
        );     
        SocketSpec spec(QString("Audio Output %1").arg(i+1), e);
        specs.push_back(spec);
    }

    for ( size_t i = 0; i < d.numBufferOutputs; ++i ){
        ConnectionEndpoint e = ConnectionEndpoint::create(
            SocketType::BufferOutbound, i, model_->getId()
        );     
        SocketSpec spec(QString("Buffer Output %1").arg(i+1), e);
        specs.push_back(spec);
    }

    for ( size_t i = 0; i < d.numMidiOutputs; ++i ){
        ConnectionEndpoint e = ConnectionEndpoint::create(
            SocketType::MidiOutbound, std::nullopt, model_->getId()
        );     
        SocketSpec spec(QString("Midi Output %1").arg(i+1), e);
        specs.push_back(spec);
    }

    if ( d.isModulator() ){
        ConnectionEndpoint e = ConnectionEndpoint::create(
            SocketType::ModulationOutbound, std::nullopt, model_->getId()
        );     
        SocketSpec spec("Modulation Output", e);
        specs.push_back(spec);
    }

    insertSockets(specs);
}

ComponentModel* ComponentNode::getModel() const {
    return model_ ;
}

json ComponentNode::serialize() const {
    json msg = GraphNode::serialize();
    msg["node_type"] = "ComponentNode" ;
    msg["componentId"] = model_->getId();

    return msg ;
}

void ComponentNode::deserialize(const json& node){
    GraphNode::deserialize(node);
    if ( node.contains("name") && node.at("name").is_string() ){
        model_->setName(QString::fromStdString(node.at("name")));
    } 
}