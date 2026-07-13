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
#include "requests/DataDescriptor.hpp"

#include <kddockwidgets/MainWindow.h>
#include <QObject>

class ComponentManager : public QObject {
    Q_OBJECT

private:
    std::map<int, ComponentModel*> models_ ;
    std::map<int, ComponentParameters*> parameters_ ;
    std::map<int, ModulationParameters*> modParameters_ ;

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
    void requestSetFile(int componentId, std::string path);

    ComponentModel* getModel(int componentId) const ;
    ComponentParameters* getParameters(int componentId) const ;
    ModulationParameters* getModulationParameters(int componentId) const ;

private:
    // on api response
    void addComponent(int componentId, ComponentType type);
    void removeComponent(int componentId);
    void setParameterValue(int componentId, ParameterType p, const json& parameterValue);
    void syncModel(const json& msg);
    void setFile(int componentId, std::string path);

    CollectionWidget* getCollectionWidget(ComponentParameters* params) const ;
    void handleCollectionApiResponse(const json& msg);

public slots:
    void onControlMessageReceived(const json& msg);
    void onDataMessageReceived(DataDescriptor header, std::vector<double> buffer);
    void onParameterEdited(int componentId, ParameterType p, ParameterValue value);
    void onCollectionEdited(CollectionRequest req );
    void onModulationDepthEdited(int componentId, ParameterType p, double depth);
    void onModulationStrategyEdited(int componentId, ParameterType p, ModulationStrategy strategy);
    void onFileSelected(int componentId, std::string path);
    void onRequestBufferData(int componentId, size_t channel);

    // for updating modulation menus
    void onConnectionAdded(const ConnectionRequest& req);
    void onConnectionRemoved(const ConnectionRequest& req);

signals:
    void componentAdded(int ComponentId, ComponentType typ);
    void componentRemoved(int componentId);
};

#endif // COMPONENT_MANAGER_HPP_