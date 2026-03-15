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

#ifndef COMPONENT_MANAGER_HPP_
#define COMPONENT_MANAGER_HPP_

#include "models/ComponentModel.hpp"
#include "models/GroupModel.hpp"
#include "views/ComponentEditor.hpp"
#include "views/GroupEditor.hpp"
#include <views/ModulationEditor.hpp>
#include "types/ComponentType.hpp"
#include "requests/CollectionRequest.hpp"
#include "widgets/CollectionWidget.hpp"
#include "widgets/ComponentParameters.hpp"
#include "requests/ConnectionRequest.hpp"

#include <QObject>

class ComponentManager : public QObject {
    Q_OBJECT

private:
    std::map<int, ComponentModel*> models_ ;
    std::map<int, ComponentEditor*> editors_ ;
    std::map<int, ModulationEditor*> modulationEditors_ ;

    std::map<int, GroupModel*> groupModels_ ;
    std::map<int, GroupEditor*> groupEditors_ ;
    std::map<int, ModulationEditor*> groupModulationEditors_ ;

    int currentGroupId_ = 0 ;

public:
    ComponentManager(QObject* parent = nullptr);
    ~ComponentManager();

    // API requests
    void requestAddComponent(ComponentType type);
    void requestRemoveComponent(int componentId);
    void requestParameterUpdate(int componentId, ParameterType p, ParameterValue v);
    void requestCollectionUpdate(CollectionRequest req);
    void requestModulationDepthUpdate(int componentId, ParameterType p, double depth);
    void requestModulationStrategyUpdate(int componentId, ParameterType p, ModulationStrategy strategy);

    void renameComponent(int id, const QString& name);
    void renameGroup(int id, const QString& name);
    
    ComponentModel* getModel(int componentId) const ;
    ComponentEditor* getEditor(int componentId) const ;
    ModulationEditor* getModulationEditor(int componentId) const ;

    GroupModel* getGroupModel(int groupId) const ;
    GroupEditor* getGroupEditor(int groupId) const ;
    ModulationEditor* getGroupModulationEditor(int groupId) const ;
    
    void showEditor(int componentId);
    void showModulationEditor(int componentId);

    void showGroupEditor(int groupId);
    void showGroupModulationEditor(int groupId);

    void createGroup(const std::vector<int> componentIds);
    void appendToGroup(int groupId, const std::vector<int> componentIds);
    void removeGroup(int groupId);

    json serialize() const ;
    
private:
    // on api response
    void addComponent(int componentId, ComponentType type);
    void removeComponent(int componentId);

    CollectionWidget* getCollectionWidget(ComponentParameters* params) const ;
    bool handleCollectionApiResponse(const json& msg);

public slots:
    void onApiDataReceived(const json& msg);
    void onParameterEdited(int componentId, ParameterType p, ParameterValue value);
    void onCollectionEdited(CollectionRequest req );
    void onModulationDepthEdited(int componentId, ParameterType p, double depth);
    void onModulationStrategyEdited(int componentId, ParameterType p, ModulationStrategy strategy);

    // for updating modulation menus
    void onConnectionAdded(const ConnectionRequest& req);
    void onConnectionRemoved(const ConnectionRequest& req);

signals:
    void componentAdded(int componentId, ComponentType type);
    void componentRemoved(int componentId);

    void componentGroupCreated(int groupId, const std::vector<int> componentIds); // new group id, components added
    void componentGroupRemoved(int groupId, const std::vector<int> componentIds); // existing group id, components removed
    void componentGroupUpdated(int groupId, const std::vector<int> componentIds); // existing group id, new component id list
};

#endif // COMPONENT_MANAGER_HPP_