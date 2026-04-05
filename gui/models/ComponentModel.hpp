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

#ifndef COMPONENT_MODEL_HPP_
#define COMPONENT_MODEL_HPP_

#include "types/ComponentType.hpp"
#include "types/ParameterType.hpp"
#include "meta/ComponentDescriptor.hpp"
#include "requests/CollectionRequest.hpp"
#include "models/ModulationModel.hpp"

#include <QObject>
#include <map>

class ComponentModel : public QObject {
    Q_OBJECT

private:
    int id_ ;
    ComponentType type_ ;
    ComponentDescriptor descriptor_ ;
    std::map<ParameterType, ParameterValue> parameters_ ;
    std::map<ParameterType, ModulationModel*> modulations_ ;

public:
    ComponentModel(int id, ComponentType typ);
    ~ComponentModel();

    int getId() const ;
    ComponentType getType() const ;
    const ComponentDescriptor& getDescriptor() const ;
    ModulationModel* getModulationModel(ParameterType p) const ;
    const std::map<ParameterType, ModulationModel*>& getModulationModels() const ;

    const ParameterValue& getParameterValue(ParameterType p) const ;
    void setParameterValue(ParameterType p, ParameterValue v, bool block = false);
    void setParameterToDefault(ParameterType p, bool block = false);

private:
    bool validParam(ParameterType p) const ;

signals:
    void parameterValueChanged(ParameterType p, ParameterValue v) const ;
    void collectionUpdated(const CollectionRequest& req);
    void modulationDepthChanged(int componentId, ParameterType p, double depth);
    void modulationStrategyChanged(int componentId, ParameterType p, ModulationStrategy strategy);
    void requestModelSync(int componentId);
        
};

#endif // COMPONENT_MODEL_HPP_