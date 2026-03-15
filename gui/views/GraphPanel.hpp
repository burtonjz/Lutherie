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

#ifndef GRAPH_PANEL_HPP_
#define GRAPH_PANEL_HPP_

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPointer>
#include <QJsonObject>
#include <vector>

#include "interfaces/ISocketLookup.hpp"
#include "managers/ConnectionManager.hpp"
#include "views/ConnectionRenderer.hpp"
#include "managers/ComponentManager.hpp"
#include "graphics/GraphNode.hpp"
#include "graphics/GroupNode.hpp"

class ComponentNode ; // forward declaration

class GraphPanel : public QGraphicsView, public ISocketLookup {
    Q_OBJECT

private:
    QGraphicsScene* scene_ ;
    ConnectionRenderer* connectionRenderer_ ;
    ConnectionManager* connectionManager_ ;
    ComponentManager* componentManager_ ;
    
    std::vector<GraphNode*> nodes_ ;
    
    // logic for managing socket hovers
    bool isDraggingConnection_ = false ;
    QPointer<SocketWidget> lastHovered_ = nullptr ;

    // hardware widgets
    GraphNode* audioOut_ ;
    GraphNode* midiIn_ ;

public:
    explicit GraphPanel(QWidget* parent = nullptr);
    ~GraphPanel();

    void addAudioOutput();
    void addMidiInput();
    
    json getComponentPositions() const ;

    void loadConnection(const json& request); 
    void loadPositions(const json& request);

    ComponentNode* getComponentNode(int componentId) const ;
    GroupNode* getGroupNode(int groupId) const ;

    GraphNode* getVisibleNode(int componentId) const ;
    GraphNode* findNodeAt(const QPointF& scenePos) const ;

    std::vector<ComponentNode*> getSelectedComponents() const ;
    std::vector<GroupNode*> getSelectedGroups() const ;

    // ISocketLookup
    SocketWidget* findSocket(SocketSpec spec) const override ;
    SocketWidget* findSocketAt(const QPointF& scenePos) const override ;

protected:
    void keyPressEvent(QKeyEvent* event) override ;
    void mousePressEvent(QMouseEvent* event) override ;
    void mouseDoubleClickEvent(QMouseEvent* event) override ;
    void wheelEvent(QWheelEvent* event) override ;
    void mouseMoveEvent(QMouseEvent* event) override ;
    void mouseReleaseEvent(QMouseEvent* event) override ;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void setupScene() ;

    void setNodeConnections(GraphNode* node);

    void drawBackground(QPainter* painter, const QRectF& rect) override ;
    void graphNodeDoubleClicked(GraphNode* widget);
    
    void handleGroupEvent();
    void handleUngroupEvent();

    void onDeletePressed();

    // context menu functions
    void onNodeRightClicked(GraphNode* node);
    void onSocketRightClicked(SocketWidget* socket);
    void startRename(GraphNode* node);

    bool isNodeNameAvailable(const QString& name, GraphNode* target = nullptr) const ;

private slots:
    void onApiDataReceived(const json& json);
    
public  slots:
    // from parent
    void onComponentSelected(ComponentType type);
    
    // from component manager
    void onComponentAdded(int componentId, ComponentType type);
    void onComponentRemoved(int componentId);

    void onComponentGroupCreated(int groupId, std::vector<int> componentIds);
    void onComponentGroupRemoved(int groupId, std::vector<int> componentIds);
    void onComponentGroupUpdated(int groupId, std::vector<int> componentIds);

    void onNodeZUpdate();

    void ondragCableParameterNeeded(SocketWidget* socket); // for completing modulation connections

signals:
    void wasModified();

};

#endif // GRAPH_PANEL_HPP_