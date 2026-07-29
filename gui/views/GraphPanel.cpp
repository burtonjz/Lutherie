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

#include "views/GraphPanel.hpp"
#include "api/ControlApiClient.hpp"
#include "app/Theme.hpp"
#include "graphics/GroupNode.hpp"
#include "managers/ConnectionManager.hpp"
#include "managers/StateManager.hpp"
#include "managers/ComponentManager.hpp"
#include "managers/GroupManager.hpp"
#include "graphics/GraphNode.hpp"
#include "graphics/SocketWidget.hpp"
#include "graphics/ComponentNode.hpp"
#include "types/ComponentType.hpp"
#include "graphics/ToastNotification.hpp"

#include <QWheelEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QGraphicsProxyWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QToolTip>
#include <QMenu>
#include <QLineEdit>

GraphPanel* GraphPanel::instance(){
    static GraphPanel panel ;
    return &panel ;
}

GraphPanel::GraphPanel(QWidget* parent):
    QGraphicsView(parent),
    isDraggingConnection_(false)
{
    setupScene();

    connectionRenderer_ = new ConnectionRenderer(scene_, ConnectionManager::instance(), this, this);

    addMidiInput();
    addAudioOutput();

    setFocusPolicy(Qt::StrongFocus);
    setEnabled(true);
    setMouseTracking(true);

    // connections

    // API Clients
    connect(
        ControlApiClient::instance(), &ControlApiClient::dataReceived, 
        this, &GraphPanel::onControlMessageReceived
    );

    // ConnectionRenderer
    connect(
        connectionRenderer_, &ConnectionRenderer::dragCableParameterNeeded,
        this, &GraphPanel::onDragCableParameterNeeded
    );

    // ComponentManager
    connect(
        ComponentManager::instance(), &ComponentManager::componentAdded,
        this, &GraphPanel::onComponentAdded
    );
    connect(
        ComponentManager::instance(), &ComponentManager::componentRemoved,
        this, &GraphPanel::onComponentRemoved
    );
    

    // GroupManager
    connect(
        this, &GraphPanel::requestGroupCreate,
        GroupManager::instance(), &GroupManager::onRequestGroupCreate
    );
    connect(
        GroupManager::instance(), &GroupManager::groupCreated,
        this, &GraphPanel::onComponentGroupCreated
    );

    connect(
        this, &GraphPanel::requestGroupUpdate,
        GroupManager::instance(), &GroupManager::onRequestGroupUpdate
    );
    connect(
        GroupManager::instance(), &GroupManager::groupUpdated,
        this, &GraphPanel::onComponentGroupUpdated
    );

    connect(
        this, &GraphPanel::requestGroupRemove,
        GroupManager::instance(), &GroupManager::onRequestGroupRemove
    );
    connect(
        GroupManager::instance(), &GroupManager::groupRemoved,
        this, &GraphPanel::onComponentGroupRemoved
    );
}

void GraphPanel::setupScene(){
    scene_ = new QGraphicsScene(this);
    scene_->setSceneRect(-2000,-2000, 4000, 4000);
    setScene(scene_);

    // view properties
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setRubberBandSelectionMode(Qt::IntersectsItemBoundingRect);

    // zooming / panning
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

void GraphPanel::setNodeConnections(GraphNode* node){
    // all graph nodes
    connect(
        node, &GraphNode::needsZUpdate, 
        this, &GraphPanel::onNodeZUpdate
    );
    connect(
        node, &GraphNode::positionChanged, 
        connectionRenderer_, &ConnectionRenderer::onNodePositionChanged
    );
    connect(
        node, &GraphNode::socketHidden,
        connectionRenderer_, &ConnectionRenderer::onSocketHidden
    );
    connect(
        node, &GraphNode::socketUnhidden,
        connectionRenderer_, &ConnectionRenderer::onSocketUnhidden
    );
    connect(
        connectionRenderer_, &ConnectionRenderer::canRemoveSocket,
        node, &GraphNode::removeSocket
    );
}

void GraphPanel::addAudioOutput(){
    audioOut_ = new PeripheralNode(AUDIO_OUT_DEVICE_ID, "Audio Output Device");
    audioOut_->insertSockets({{
        .type = SocketType::SignalInbound, 
        .name = "Audio In",
        .idx  = 0
    }});
    
    audioOut_->addToScene(scene_);
    audioOut_->moveBy(200, 0);
    nodes_.push_back(audioOut_);
    
    setNodeConnections(audioOut_);

    SPDLOG_INFO("Created audio output node named {} at position {},{}", 
        audioOut_->getName().toStdString(), audioOut_->pos().x(), audioOut_->pos().y());
}

void GraphPanel::addMidiInput(){
    midiIn_ = new PeripheralNode(MIDI_IN_DEVICE_ID, "MIDI Input Device");
    midiIn_->insertSockets({{
        .type = SocketType::MidiOutbound, 
        .name = "MIDI Out"
    }});

    midiIn_->addToScene(scene_);
    midiIn_->moveBy(-200,0);
    nodes_.push_back(midiIn_);
    
    setNodeConnections(midiIn_);

    SPDLOG_INFO("Created midi input node named {} at position {},{}",
        midiIn_->getName().toStdString(), midiIn_->pos().x(), midiIn_->pos().y());
}

GraphNode* GraphPanel::getVisibleNode(int componentId) const {
    for ( auto n : nodes_ ){
        auto cNode = dynamic_cast<ComponentNode*>(n);
        if ( cNode && cNode->isVisible() ){
            if ( cNode->getModel()->getId() == componentId ){
                return cNode ;
            }
        }
        auto gNode = dynamic_cast<GroupNode*>(n);
        if ( gNode && gNode->contains(componentId) ){
            return gNode ;
        }
    }
    return nullptr ;
}

GraphNode* GraphPanel::findNodeAt(const QPointF& scenePos) const {
    auto items = scene_->items(scenePos);
    for ( auto item : items ){
        if ( GraphNode* node = dynamic_cast<GraphNode*>(item) ){
            return node ;
        }
    }
    return nullptr ;
}

ComponentNode* GraphPanel::getComponentNode(int componentId) const {
    for ( auto n : nodes_ ){
        auto cNode = dynamic_cast<ComponentNode*>(n);
        if ( cNode ){
            if ( cNode->getModel()->getId() == componentId ){
                return cNode ;
            }
        }
    }
    return nullptr ;
}

GroupNode* GraphPanel::getGroupNode(int groupId) const {
    for ( auto n : nodes_ ){
        auto gNode = dynamic_cast<GroupNode*>(n);
        if ( gNode ){
            if ( gNode->getId() == groupId ){
                return gNode ;
            }
        }
    }
    return nullptr ;
}

json GraphPanel::serializeNodes() const {
    json nodes ;
    for ( auto n : nodes_ ){
        nodes.push_back(n->serialize());
    }
    return nodes ;
}

void GraphPanel::deserializeNodes(const json& nodes){
    if ( ! nodes.is_array() ){
        SPDLOG_WARN("nodes are not in expected json format.");
        return ;
    }

    for ( const auto& n : nodes ) {        
        if ( ! n.contains("node_type") || ! n.at("node_type").is_string() ){
            SPDLOG_WARN("serialized node does not have a type identifier. Ignoring object.");
            continue ;
        }

        const std::string nodeType = n.at("node_type");

        if ( nodeType == "ComponentNode" ){
            if ( ! n.contains("componentId") || ! n.at("componentId").is_number() ){
                SPDLOG_WARN("component node does not have id specified");
                continue ;
            }
            auto c = getComponentNode(n.at("componentId"));
            if ( ! c){
                SPDLOG_WARN("component node not found for id {}");
                continue ;
            }
            c->deserialize(n);
        } else if ( nodeType == "PeripheralNode" ){
            if ( !n.contains("deviceId") || ! n.at("deviceId").is_number() ){
                SPDLOG_WARN("peripheral node does not have a defined deviceId");
                continue ;
            }

            PeripheralNode* p = nullptr ;
            if ( n.at("deviceId") == MIDI_IN_DEVICE_ID ){
                p = midiIn_ ;
            } else if ( n.at("deviceId") == AUDIO_OUT_DEVICE_ID ){
                p = audioOut_ ;
            }
            if ( p ){
                p->deserialize(n);
            } else {
                SPDLOG_WARN("Invalid deviceId specified: {}", n.at("deviceId").dump());
            }
        } else if ( nodeType == "GroupNode" ){
            if ( ! n.contains("componentIds") || ! n.at("componentIds").is_array() ){
                continue ;
            }

            std::vector<int> ids ;
            for ( const auto& id : n.at("componentIds") ){
                if ( ! id.is_number() ){
                    SPDLOG_WARN("componentId in array is malformed. Expected number format, but got {}",
                        id.dump());
                    continue ;
                }
                ids.push_back(id);
            }

            if ( ids.size() > 1 ){
                emit requestGroupCreate(ids, n);
            } else {
                SPDLOG_WARN("Group node does not contain at least 2 valid component ids.");
            }
        }
    }
}

std::vector<ComponentNode*> GraphPanel::getSelectedComponents() const {
    auto selectedItems = scene_->selectedItems() ;
    std::vector<ComponentNode*> nodes ;

    for ( QGraphicsItem* item: selectedItems ){
        ComponentNode* node = dynamic_cast<ComponentNode*>(item);
        if ( node ){
            nodes.push_back(node);
        }
    }
    return nodes ;
}

std::vector<GroupNode*> GraphPanel::getSelectedGroups() const {
    auto selectedItems = scene_->selectedItems() ;
    std::vector<GroupNode*> nodes ;

    for ( QGraphicsItem* item: selectedItems ){
        GroupNode* node = dynamic_cast<GroupNode*>(item);
        if ( node ){
            nodes.push_back(node);
        }
    }
    return nodes ;
}

SocketWidget* GraphPanel::findSocket(SocketSpec spec) const {
    GraphNode* w = nullptr ;

    // first, find the corresponding GraphNode
    if ( !spec.componentId.has_value() ){ 
        if ( spec.type == SocketType::SignalInbound ){
            w = audioOut_ ;
        } else if ( spec.type == SocketType::MidiOutbound ){
            w = midiIn_ ;
        } 
    } else {
        w = getVisibleNode(spec.componentId.value());
    }

    if ( !w ){ 
        SPDLOG_WARN("Could not find node matching search criteria.");
        return nullptr ;
    }

    // search its sockets
    for ( auto s : w->getSockets() ){
        if ( s->matches(spec) ){
            return s ;
        }
    }

    SPDLOG_WARN("Could not find socket matching search criteria.");
    return nullptr ;
}

SocketWidget* GraphPanel::findSocketAt(const QPointF& scenePos) const {
    auto items = scene_->items(scenePos);
    for ( auto item : items ){
        if ( SocketWidget* socket = dynamic_cast<SocketWidget*>(item) ){
            return socket ;
        }
    }
    return nullptr ;
}

void GraphPanel::keyPressEvent(QKeyEvent* event){
    // scene/proxy gets priority first if focused
    if ( scene_->focusItem() ){
        QGraphicsView::keyPressEvent(event);
        return ;
    }

    switch (event->key()){
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            onDeletePressed();
            break ;
        case Qt::Key_Escape:
            connectionRenderer_->cancelDrag();
            break ;
        case Qt::Key_G:
            if ( event->modifiers() & Qt::ControlModifier ){
                handleGroupEvent();
            }
            break ;
        case Qt::Key_U:
            if ( event->modifiers() & Qt::ControlModifier ){
                handleUngroupEvent();
            }
            break ;
        default:
            QGraphicsView::keyPressEvent(event);
    }
}

void GraphPanel::mouseMoveEvent(QMouseEvent* event){
    QPointF scenePos = mapToScene(event->pos());
    SocketWidget* w = findSocketAt(scenePos);

    // resolve hover events for socket widgets
    if ( lastHovered_ ){
        lastHovered_->setHovered(false);
        lastHovered_ = nullptr ;
    }

    if ( w ){
        w->setHovered(true);
        lastHovered_ = w ;
    }
    
    // handle socket connection dragging
    if ( isDraggingConnection_ ){
        connectionRenderer_->updateDrag(scenePos);

        // manually show tool tip if hovering
        if (w && !w->toolTip().isEmpty()){
            QToolTip::showText(QCursor::pos(), w->toolTip());
            return ;
        }

        // hide tool tip if no longer found
        QToolTip::hideText();

        event->accept();
        return ;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void GraphPanel::mousePressEvent(QMouseEvent* event){
    QPointF scenePos = mapToScene(event->pos());

    if ( event->button() == Qt::LeftButton ){
        if ( SocketWidget* w = findSocketAt(scenePos) ){
            isDraggingConnection_ = true ;
            connectionRenderer_->startDrag(w);
            event->accept();
            return ;
        }
    }

    QGraphicsView::mousePressEvent(event); // pass event through
}

void GraphPanel::mouseDoubleClickEvent(QMouseEvent* event){
    QPointF scenePos = mapToScene(event->pos());

   
    QGraphicsItem* item = scene()->itemAt(scenePos, transform());
    while (item){
        // launch the component editor ; 
        if ( GraphNode* w = dynamic_cast<GraphNode*>(item)){ 
            graphNodeDoubleClicked(w);
            return ;
        } 
        item = item->parentItem();
    }
}

void GraphPanel::mouseReleaseEvent(QMouseEvent* event){
    QPointF scenePos = mapToScene(event->pos());

    if (event->button() == Qt::LeftButton && isDraggingConnection_ ) {
        isDraggingConnection_ = false;
        connectionRenderer_->finishDrag(scenePos);
        event->accept();
        return ;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void GraphPanel::contextMenuEvent(QContextMenuEvent *event){
    QPointF scenePos = mapToScene(event->pos());

    // right clicking on a socket
    if ( SocketWidget* w = findSocketAt(scenePos) ){
        onSocketRightClicked(w);
        return ;
    }

    // right clicking on a graph node
    if ( GraphNode* n =  findNodeAt(scenePos) ){
        onNodeRightClicked(n);
        return ;
    }

}

void GraphPanel::onNodeRightClicked(GraphNode* node){
    QMenu menu ;
    QMenu* showMenu = new QMenu("Show", &menu);
    QMenu* socketMenu = menu.addMenu("Sockets");
 
    // show menu
    if ( auto c = dynamic_cast<ComponentNode*>(node) ){
        if ( c->getModel()->getDescriptor().controllableParameters.size() > 0 ){
            QAction* openParams = showMenu->addAction("Parameter Controls");
            connect ( openParams, &QAction::triggered, [this,c](){
                emit requestShowParameters(c->getModel()->getId());
            });
        }
    } else if ( auto g = dynamic_cast<GroupNode*>(node) ){
        QAction* openParams = showMenu->addAction("Parameter Controls");
        connect ( openParams, &QAction::triggered, [this,g](){
            emit requestShowGroupParameters(g->getId());
        });
    }
    
    if ( auto c = dynamic_cast<ComponentNode*>(node) ){
        if ( c->getModel()->getDescriptor().modulatableParameters.size() > 0 ){
            QAction* openMod = showMenu->addAction("Modulation Controls");
            connect ( openMod, &QAction::triggered, [this,c](){
                emit requestShowModulation(c->getModel()->getId());
            });
        }
    } else if ( auto g = dynamic_cast<GroupNode*>(node) ){
        QAction* openMod = showMenu->addAction("Modulation Controls");
        connect ( openMod, &QAction::triggered, [this,g](){
            emit requestShowGroupModulation(g->getId());
        });
    }

    if ( showMenu->actions().size() > 0 ){
        menu.addMenu(showMenu);
    }

    // socket menu
    QAction* unhideAll = new QAction("Unhide All", socketMenu);
    connect(unhideAll, &QAction::triggered, [node]{
        node->unhideAllSockets();
    });
    socketMenu->addAction(unhideAll);

    QAction* hideDisconnected = new QAction("Hide Disconnected", socketMenu);
    connect(hideDisconnected, &QAction::triggered, [node](){
        node->hideDisconnectedSockets();
    });
    socketMenu->addAction(hideDisconnected);

    QAction* hideInternal = new QAction("Hide Internal Connections", socketMenu);
    connect(hideInternal, &QAction::triggered, [this, node](){
        for ( auto s : node->getSockets() ){
            if ( !s->isVisible() || !s->hasConnection() ) continue ;
            bool internalOnly = true ;
            for ( auto connection : connectionRenderer_->getSocketConnections(s) ){
                if ( 
                    connection->getInboundSocket()->getParent() != node ||
                    connection->getOutboundSocket()->getParent() != node
                ){
                    internalOnly = false ;
                    break ;
                }
            }
            if ( internalOnly ){
                node->hideSocket(s);
            }   
        }
    });
    socketMenu->addAction(hideInternal);

    socketMenu->addSeparator();
    for ( auto s : node->getHiddenSockets() ){
        QAction* showSocket = new QAction("Unhide " + s->getSpec().name, socketMenu);
        connect ( showSocket, &QAction::triggered, [node, s]{
            node->unhideSocket(s);
        });
        socketMenu->addAction(showSocket);
    }

    // MISC ACTIONS

    // rename
    QAction* rename = new QAction("Rename", &menu);
    connect (rename, &QAction::triggered, [this, node](){
        startRename(node);
    });
    menu.addAction(rename);

    // export audio buffer
    if ( ComponentNode* componentNode = dynamic_cast<ComponentNode*>(node) ){
        const ComponentDescriptor& descriptor = componentNode->getModel()->getDescriptor();
        if ( descriptor.numBufferOutputs > 0 ){
            QAction* exportBuffer = new QAction("Export Buffer", &menu);
            connect(exportBuffer, &QAction::triggered, [this, componentNode](){
                requestSaveBuffer(componentNode->getModel()->getId());
            });
            menu.addAction(exportBuffer);
        }
    }

    menu.exec(QCursor::pos());
}

void GraphPanel::onSocketRightClicked(SocketWidget* socket){
    QMenu menu ;

    QAction* disconnectAll = new QAction("Disconnect All",&menu);
    connect(disconnectAll, &QAction::triggered, [this, socket]() 
        { connectionRenderer_->requestRemoveSocketConnections(socket);}
    );

    QMenu* disconnectMenu = new QMenu("Disconnect",&menu);
    GraphNode* node = socket->getParent();
    bool isInbound = socket->isInbound();

    for ( const auto c : connectionRenderer_->getNodeConnections(node)){
        if ( ! c->involvesSocket(socket) ) continue ;
        GraphNode* other ;
        QString s ;
        if ( isInbound ){
            other = c->getOutboundSocket()->getParent();
            s = other->getName() + ": " + c->getOutboundSocket()->getSpec().name ;
        } else {
            other = c->getInboundSocket()->getParent();
            s = other->getName() + ": " + c->getInboundSocket()->getSpec().name ;
        }
        QAction* disconnectOne = new QAction(s,disconnectMenu);
        connect( disconnectOne, &QAction::triggered, [this, c](){
            connectionRenderer_->requestRemoveConnection(c);
        });
        disconnectMenu->addAction(disconnectOne);
    }

    QAction* socketHide = new QAction("Hide Socket", &menu);
    connect(socketHide, &QAction::triggered, [node, socket](){
        node->hideSocket(socket);
    });

    menu.addAction(socketHide);

    menu.addMenu(disconnectMenu);
    menu.addAction(disconnectAll);

    menu.exec(QCursor::pos());
}

void GraphPanel::startRename(GraphNode* node ){
    if ( !node ) return ;

    QGraphicsTextItem* text = node->getNameItem() ;
    QRectF rect = text->boundingRect();
    QPointF scenePos = text->mapToScene(rect.topLeft());

    QLineEdit* edit = new QLineEdit();
    edit->setText(node->getName());
    edit->selectAll();
    edit->setFont(text->font());

    QGraphicsProxyWidget* proxy = scene_->addWidget(edit);
    proxy->setPos(scenePos);
    proxy->resize(rect.size());
    proxy->setFocus();
    edit->setFocus();
    text->hide();

    connect(edit, &QLineEdit::textChanged, [this, node, edit](const QString& text)
    {
        bool available = text.trimmed().isEmpty() || isNodeNameAvailable(text.trimmed(), node);
        edit->setStyleSheet(available ? "" : "color: red; background: transparent;");
    });

    connect(edit, &QLineEdit::editingFinished, [this, node, text, edit, proxy](){
        QString newName = edit->text().trimmed();
        if ( ! newName.isEmpty() ){
            if ( isNodeNameAvailable(newName, node) ){
                updateModelName(node, newName);
            } else {
                ToastNotification::show(scene_, this, 
                    "Cannot name widget '" + newName + "'. Name is unavailable.");
            }
        } 
        text->show();
        scene_->removeItem(proxy);
        proxy->deleteLater();
    });
}

void GraphPanel::requestSaveBuffer(int componentId){
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Buffer"),
        QDir::homePath(),
        tr("Audio Files (*.wav *.aiff *.mp3);;All Files (*)")
    );
    if (filePath.isEmpty()) {
        return ; 
    }
    ComponentManager::instance()->requestSaveBuffer(componentId, filePath.toStdString());
}

void GraphPanel::wheelEvent(QWheelEvent* event){
    if ( event->angleDelta().y() > 0 ){
        scale(Theme::GRAPH_WHEEL_SCALE_FACTOR, Theme::GRAPH_WHEEL_SCALE_FACTOR);
    } else {
        scale( 1.0 / Theme::GRAPH_WHEEL_SCALE_FACTOR, 1.0 / Theme::GRAPH_WHEEL_SCALE_FACTOR );
    }
}

bool GraphPanel::isNodeNameAvailable(const QString& name, GraphNode* target) const {
    for ( const auto& node : nodes_ ){
        if ( node == target ) continue ;
        if ( node && node->getName() == name ){
            return false ;
        }
    }
    return true ;
}

void GraphPanel::updateModelName(GraphNode* node, const QString& name){
    if ( ComponentNode* cn = dynamic_cast<ComponentNode*>(node) ){
        cn->getModel()->setName(name);
        return ;
    } 

    if ( GroupNode* gn = dynamic_cast<GroupNode*>(node) ){
        gn->getModel()->setName(name);
        return ;
    } 
}

void GraphPanel::drawBackground(QPainter* painter, const QRectF& rect){
    // Draw Grid
    painter->setPen(QPen(Theme::GRAPH_GRID_COLOR, 1));

    qreal left = int(rect.left()) - (int(rect.left()) % int(Theme::GRAPH_GRID_SIZE));
    qreal top = int(rect.top()) - (int(rect.top()) % int(Theme::GRAPH_GRID_SIZE));

    QVarLengthArray<QLineF, 100> lines ;

    for (qreal x = left; x < rect.right(); x+= Theme::GRAPH_GRID_SIZE){
        lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    }

    for (qreal y = top; y < rect.bottom(); y += Theme::GRAPH_GRID_SIZE){
        lines.append(QLineF(rect.left(), y, rect.right(), y));
    }

    painter->drawLines(lines.data(), lines.size());

}

void GraphPanel::onControlMessageReceived(const json& msg){
    QString action = QString::fromStdString(msg["action"]) ;

    if ( action == "load_patch" ){
        if ( msg.at("status") == "success"){
            if ( msg.contains("nodes") ){
                deserializeNodes(msg.at("nodes"));
            }
        }
        return ;
    }

    if ( action == "get_audio_configuration" ){
        if ( msg.at("status") == "success" ){
            if ( msg.contains("output_channels") ){
                onAudioChannelsUpdated(msg.at("output_channels"));
            }
        }
        return ;
    }
}

void GraphPanel::onComponentSelected(ComponentType type){
    ComponentManager::instance()->requestAddComponent(type);
}

void GraphPanel::onComponentAdded(int componentId, ComponentType type){
    auto* m = ComponentManager::instance()->getModel(componentId);

    if ( !m ){
        SPDLOG_WARN("cannot create component node. No model found with componentId {}", componentId);
        return ;
    }

    QString name = QString::fromStdString(m->getDescriptor().name) ;
    QString baseName = name ;
    int count = 1 ;
    while ( !isNodeNameAvailable(name) ){
        name = baseName + " " + QString::number(count++);
    }
    if ( count > 1 ) m->setName(name);
    
    auto n = new ComponentNode(m);
    nodes_.push_back(n);

    setNodeConnections(n);
    n->addToScene(scene_);
    n->setPos(0,0); // TODO: dynamically place the module somewhere currently empty on the scene
}

void GraphPanel::onComponentRemoved(int componentId){
    auto n = getComponentNode(componentId);
    if ( !n ){
        SPDLOG_WARN("Cannot remove component with id {}. Node does not exist.");
        return ;
    }

    nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), n), nodes_.end());
    scene_->removeItem(n);
    n->deleteLater();
}

void GraphPanel::onComponentGroupCreated(int groupId, std::vector<int> componentIds, std::optional<json> deserialized){
    auto* model = GroupManager::instance()->getModel(groupId);
    model->setName(QString("Group %1").arg(groupId));

    auto gNode =  new GroupNode(model);
    nodes_.push_back(gNode);
    gNode->addToScene(scene_);

    // connections 
    setNodeConnections(gNode);

    for ( const auto id : componentIds ){
        gNode->add(getComponentNode(id));
    }

    connectionRenderer_->onComponentGroup(componentIds);

    if ( deserialized.has_value() ){
        gNode->deserialize(deserialized.value());
    }
}

void GraphPanel::onComponentGroupRemoved(int groupId, std::vector<int> componentIds){
    auto gNode = getGroupNode(groupId);

    if ( !gNode ){
        SPDLOG_WARN("Node with groupId {} not found. Cannot delete.", groupId);
        return ;
    }

    gNode->removeAll();
    nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), gNode), nodes_.end());
    scene_->removeItem(gNode);
    gNode->deleteLater();

    connectionRenderer_->onComponentGroup(componentIds);
}

void GraphPanel::onComponentGroupUpdated(int groupId, std::vector<int> componentIds){
    auto gNode = getGroupNode(groupId);

    if ( !gNode ){
        SPDLOG_WARN("Node with groupId {} not found. Cannot delete.", groupId);
        return ;
    }

    gNode->removeAll();
    for ( const auto id : componentIds ){
        gNode->add(getComponentNode(id));
    }

    connectionRenderer_->onComponentGroup(componentIds);
}


void GraphPanel::graphNodeDoubleClicked(GraphNode* widget){
    if ( auto c = dynamic_cast<ComponentNode*>(widget) ){
        emit requestShowParameters(c->getModel()->getId());
        return ;
    }

    if ( auto g = dynamic_cast<GroupNode*>(widget) ){
        emit requestShowGroupParameters(g->getId());
        return ;
    }
}

void GraphPanel::onDeletePressed(){
    if ( StateManager::instance()->isRunning() ){
        ToastNotification::show(scene_, this, "Cannot delete components while the engine is running.");
        return ;
    }
    // delete all selected components
    for ( const auto c : getSelectedComponents() ){
        ComponentManager::instance()->requestRemoveComponent(c->getModel()->getId());
    }
}

void GraphPanel::handleGroupEvent(){
    std::vector<int> groupIds ;
    std::vector<int> componentIds ;

    for ( const auto& g : getSelectedGroups() ){
        groupIds.push_back(g->getId());
    }

    // sort components by name for initial group
    auto selected = getSelectedComponents();
    sort(selected.begin(), selected.end(), [](const ComponentNode* a, const ComponentNode* b){
        return a->getName() < b->getName();
    });
    for ( const auto& c: selected ){
        componentIds.push_back(c->getModel()->getId());
    }

    if ( groupIds.size() == 0 && componentIds.size() == 0 ) return ;
    if ( groupIds.size() == 0 && componentIds.size() == 1 ) return ;
    
    // case 1: no groups selected, create new group
    if ( groupIds.size() == 0 ){
       emit requestGroupCreate(componentIds);
    }

    // case 2: one group selected, add into group
    if ( groupIds.size() == 1 ){
        emit requestGroupUpdate(groupIds[0], componentIds);
    }
}

void GraphPanel::handleUngroupEvent(){
    for ( const auto& g : getSelectedGroups() ){
        emit requestGroupRemove(g->getId());
    }
}

void GraphPanel::onNodeZUpdate(){
    GraphNode* node = dynamic_cast<GraphNode*>(sender());
    int maxZ = 0 ;
    for ( auto n : nodes_ ){
        if ( n != node && n->zValue() > maxZ ){
            maxZ = n->zValue();
        }
    }

    if ( maxZ != 0 && node->zValue() == maxZ ) return ;

    // nodes in front of cables in front of sockets
    node->setZValue( maxZ + 1 );

    auto cables = connectionRenderer_->getNodeConnections(node);
    for ( auto* cable : cables ){
        cable->setZValue( maxZ  + 0.9 ); 
    }

    auto sockets = node->getSockets();
    for ( auto* socket: sockets){
        socket->setZValue(maxZ + 0.8);
    }
}

void GraphPanel::onDragCableParameterNeeded(SocketWidget* socket){
    if ( ! socket ){
        SPDLOG_WARN("drag cable parameter requested for an invalid socket. Cancelling drag.");
        connectionRenderer_->cancelDrag();
        return ;
    }

    if ( ! socket->getSpec().componentId.has_value() ){
        SPDLOG_WARN("drag cable inbound socket does not have a defined componentId. Cancelling drag.");
        connectionRenderer_->cancelDrag();
        return ;
    }

    QMenu menu ;

    QAction* header = menu.addAction("Select Parameter");
    header->setEnabled(false);
    menu.addSeparator();

    int id = socket->getSpec().componentId.value() ;

    auto params = ComponentManager::instance()
        ->getModel(id)
        ->getDescriptor().modulatableParameters ;

    auto existing = ConnectionManager::instance()
        ->getModulationConnections(id);

    auto depthExisting = ConnectionManager::instance()
        ->getModulationDepthConnections(id);

    bool hasActions = false ;
    for ( const auto& p : params ){
        bool modExists = std::find(existing.begin(), existing.end(), p) != existing.end();
        bool depthExists = std::find(depthExisting.begin(), depthExisting.end(), p) != depthExisting.end();
        if ( ! modExists ){
            QAction* param = new QAction(
                QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(p, name))),
                &menu
            );
            menu.addAction(param);
            hasActions = true ;
        }   
        if ( modExists && ! depthExists ){
            QAction* param = new QAction(
                QString::fromStdString(std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)) + " depth"),
                &menu
            );
            menu.addAction(param);
            hasActions = true ;
        }    
    }

    if ( ! hasActions ){ 
        connectionRenderer_->cancelDrag();
        ToastNotification::show(scene_, this, "All modulation slots are full.");
        return ;
    }

    QAction* selected = menu.exec(QCursor::pos());
    if ( selected ){
        auto str = selected->text().toStdString();
        ParameterType p ;
        if ( str.find("depth") != std::string::npos ){
            str.erase(str.find(" depth"), 6);
            p = stringToParameter(str);
            connectionRenderer_->setDragCableParameter(p, true);
        } else {
            p = stringToParameter(str);
            connectionRenderer_->setDragCableParameter(p);
        }
    } else {
        connectionRenderer_->cancelDrag();
    }
}

void GraphPanel::onAudioChannelsUpdated(size_t numChannels){
    auto sockets = audioOut_->getSockets();
    size_t oldSize = sockets.size();
    if ( oldSize == numChannels ) return ;

    // new output peripheral has less channels
    if ( oldSize > numChannels ){
        for ( auto s : sockets ){
            if ( !s ) continue ;
            auto spec = s->getSpec();
            if ( spec.idx.has_value() && spec.idx.value() >= numChannels ){
                connectionRenderer_->requestRemoveSocket(s);
            }
        }
        return ;
    }

    // otherwise, there are more channels
    std::vector<SocketSpec> specs ;
    for ( size_t i = oldSize ; i < numChannels ; ++i ){
        specs.push_back({
            .type = SocketType::SignalInbound, 
            .name = "Audio In " + QString::number(i),
            .idx  = i    
        });
    }
    
    audioOut_->insertSockets(specs);
    audioOut_->addToScene(scene_);
}
