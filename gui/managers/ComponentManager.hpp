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
#include "types/ComponentType.hpp"
#include "requests/CollectionRequest.hpp"
#include "widgets/CollectionWidget.hpp"
#include "widgets/ComponentParameters.hpp"
#include "widgets/ModulationParameters.hpp"
#include "requests/ConnectionRequest.hpp"

#include <kddockwidgets/MainWindow.h>
#include <QObject>

class ComponentManager : public QObject {
    Q_OBJECT

private:
    std::map<int, ComponentModel*> models_ ;
    std::map<int, ComponentParameters*> parameters_ ;
    std::map<int, ModulationParameters*> modParameters_ ;

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
    void requestModelSync(int componentId);

    void renameComponent(int id, const QString& name);
    void renameGroup(int id, const QString& name);
    
    ComponentModel* getModel(int componentId) const ;
    ComponentParameters* getParameters(int componentId) const ;
    ModulationParameters* getModulationParameters(int componentId) const ;
    
    void showParameters(int componentId);
    void showModulation(int componentId);
    void showGroupParameters(int groupId);
    void showGroupModulation(int groupId);

    int createGroup(const std::vector<int> componentIds, bool block = false);
    void appendToGroup(int groupId, const std::vector<int> componentIds);
    void removeGroup(int groupId);
    
private:
    // on api response
    void addComponent(int componentId, ComponentType type);
    void removeComponent(int componentId);
    void setParameterValue(int componentId, ParameterType p, const json& parameterValue);
    void syncModel(const json& msg);

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
    void componentAdded(int ComponentId, ComponentType typ);
    void componentRemoved(int componentId);

    void componentGroupCreated(int groupId, const std::vector<int> componentIds); // new group id, components added
    void componentGroupRemoved(int groupId, const std::vector<int> componentIds); // existing group id, components removed
    void componentGroupUpdated(int groupId, const std::vector<int> componentIds); // existing group id, new component id list
};

#endif // COMPONENT_MANAGER_HPP_