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
{}

void GroupNode::clear(){
    for ( auto* node : children_ ){
        if ( !node ) continue ;
        node->show();
    }
    removeSockets();
}

void GroupNode::addComponent(ComponentNode* node){
    if ( !node || includes(node) ) return ;
    children_.push_back(node);
    addComponentSockets(node);
    node->hide();
}

void GroupNode::removeComponent(ComponentNode* node){
    if ( !node || includes(node) ) return ;
    node->show();
    children_.erase(std::remove(
        children_.begin(), children_.end(), node
        ), children_.end() 
    );
    removeComponentSockets(node);
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

GroupModel* GroupNode::getModel() const {
    return model_ ;
}


void GroupNode::addComponentSockets(ComponentNode* node){
    if ( !node ) return ;

    std::vector<SocketSpec> specs ;
    for ( auto s : node->getSockets() ){
        auto spec = s->getSpec();
        spec.setName(spec.name() + " (" + node->getName() + ")");
        specs.push_back(spec);
    }

    insertSockets(specs);
}

void GroupNode::removeComponentSockets(ComponentNode* node){
    if ( !node ) return ;

    std::vector<SocketSpec> specs ;
    for ( auto s : node->getSockets() ){
        specs.push_back(s->getSpec());
    }

    removeSockets(specs);
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