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

#include <QGraphicsSceneMouseEvent>
#include <vector>

ComponentNode::ComponentNode(ComponentModel* model, const QString& name, QGraphicsItem* parent): 
    GraphNode(name, parent),
    model_(model),
    specs_()
{
    auto d = model_->getDescriptor();
    
    // create sockets from descriptor
    if ( d.modulatableParameters.size() > 0 ){
        specs_.push_back({
            .type        =SocketType::ModulationInbound, 
            .name        = "Inbound Modulation", 
            .componentId = model_->getId()
        });
    }
    
    for (int i = 0; i < d.numAudioInputs; ++i){
        specs_.push_back({
            .type        = SocketType::SignalInbound, 
            .name        = QString("Audio Input %1").arg(i+1), 
            .componentId = model_->getId(),
            .idx         = i
        });
    }

    for (int i = 0; i < d.numMidiInputs; ++i){
        specs_.push_back({
            .type = SocketType::MidiInbound, 
            .name = QString("MIDI Input %1").arg(i+1),
            .componentId = model_->getId()
        });
    }

    for (int i = 0; i < d.numAudioOutputs; ++i){
        specs_.push_back({
            .type = SocketType::SignalOutbound, 
            .name = QString("Audio Output %1").arg(i+1), 
            .componentId = model_->getId(),
            .idx = i
        });
    }

    for (int i = 0; i < d.numMidiOutputs; ++i){
        specs_.push_back({
            .type = SocketType::MidiOutbound, 
            .name = QString("MIDI Output %1").arg(i+1),
            .componentId = model_->getId()
        });
    }

    if ( d.isModulator()){
        specs_.push_back({
            .type = SocketType::ModulationOutbound, 
            .name = QString("Modulation Output"),
            .componentId = model_->getId()
        });
    }

    createSockets(specs_);

}

ComponentModel* ComponentNode::getModel() const {
    return model_ ;
}

const std::vector<SocketSpec>& ComponentNode::getSpecs() const {
    return specs_ ;
}

json ComponentNode::serialize() const {
    json msg = GraphNode::serialize();
    msg["node_type"] = "ComponentNode" ;
    msg["componentId"] = model_->getId();

    return msg ;
}

void ComponentNode::deserialize(const json& node){
    GraphNode::deserialize(node);
}