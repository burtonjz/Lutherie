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

#include "GroupNode.hpp"
#include <QGraphicsScene>
#include <spdlog/spdlog.h>

GroupNode::GroupNode(GroupModel* model, QGraphicsItem* parent):
    GraphNode(model->getName(), parent),
    model_(model)
{
}

void GroupNode::add(ComponentNode* node){
    if ( !node || includes(node) ){
        SPDLOG_WARN("ignoring GroupNode Component add with null pointer");
        return ;
    } 
    children_.push_back(node);
    addSockets(node);
    node->hide();
}

// void GroupNode::remove(ComponentNode* node){
//     if ( !node || !includes(node) ) return ;
//     children_.erase(std::remove(children_.begin(), children_.end(), node), children_.end());
//     removeSockets(node);
//     node->show();
// }

void GroupNode::clear(){
    children_.clear();
    removeSockets();
}

bool GroupNode::includes(ComponentNode* node) const {
    auto it = std::find(children_.begin(), children_.end(), node);
    return it != children_.end() ;
}

bool GroupNode::includes(int componentId) const {
    for ( const auto& c : children_ ){
        if ( c->getModel()->getId() == componentId ){
            return true ;
        }
    }
    return false ;
}

size_t GroupNode::getNumComponents() const {
    return children_.size();
}

int GroupNode::getId() const {
    return model_->getId() ;
}

GroupModel* GroupNode::getModel() const {
    return model_ ;
}


void GroupNode::addSockets(ComponentNode* node){
    for ( auto cSocket : node->getSockets() ){
        auto spec = cSocket->getSpec();
        spec.setName(node->getName() + " " + spec.name());
        SocketWidget* socket = new SocketWidget(spec, this);
        sockets_.push_back(socket);  
        if ( scene() ) scene()->addItem(socket);
    }

    layoutSockets();
    reorderSockets();
    positionSockets(scenePos());
}

json GroupNode::serialize() const {
    json msg = GraphNode::serialize();
    msg["node_type"] = "GroupNode" ;
    msg["groupId"] = model_->getId() ;
    auto& components = msg["componentIds"];
    for ( const auto& c : children_ ){
        components.push_back(c->getModel()->getId() );
    }

    return msg ;
}

void GroupNode::deserialize(const json& node){
    GraphNode::deserialize(node);
    if ( node.contains("name") && node.at("name").is_string() ){
        model_->setName(QString::fromStdString(node.at("name")));
    } 
}