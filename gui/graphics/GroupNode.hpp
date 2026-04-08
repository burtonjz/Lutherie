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

#ifndef GROUP_NODE_HPP_
#define GROUP_NODE_HPP_

#include "graphics/GraphNode.hpp"
#include "graphics/ComponentNode.hpp"

#include <QGraphicsItem>

class GroupNode : public GraphNode {
    Q_OBJECT

private:
    int groupId_ ; 
    std::vector<ComponentNode*> children_ ;
    
public:
    explicit GroupNode(int groupId, const QString& name, QGraphicsItem* parent = nullptr);

    void add(ComponentNode* node);
    void remove(ComponentNode* node);
    void removeAll();

    bool contains(ComponentNode* node) const ;
    bool contains(int componentId) const ;
    
    size_t getNumComponents() const ;

    int getId() const ; 

    json serialize() const override ;
    // virtual void deserialize(const json& node) override ;

    void requestRename(const QString& name ) override ;

private:
    void addSockets(ComponentNode* node);
    void removeSockets(ComponentNode* node);

signals:
    // signal group events to connection renderer
    void SocketGrouped(SocketSpec spec);
    void SocketUngrouped(SocketSpec spec);

    void requestGroupRename(int groupId, QString name);

};

#endif // GROUP_NODE_HPP_