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
#include <QString>
#include <map>

class ComponentModel : public QObject {
    Q_OBJECT

private:
    int id_ ;
    ComponentType type_ ;
    ComponentDescriptor descriptor_ ;
    std::map<ParameterType, ParameterValue> parameters_ ;
    std::map<ParameterType, std::pair<ParameterValue,ParameterValue>> paramRanges_ ;
    std::map<ParameterType, ModulationModel*> modulations_ ;
    QString name_ ;
    std::optional<std::string> file_ ;

    std::unordered_map<size_t, std::vector<double>> buffers_ ;
    std::unordered_map<size_t, std::pair<size_t, ComponentModel*>> upstream_ ;

public:
    ComponentModel(int id, ComponentType typ);
    ~ComponentModel();

    int getId() const ;
    ComponentType getType() const ;

    const QString& getName() const ;
    void setName(QString name);

    std::string getFile() const ;
    void setFile(std::string name);

    bool hasBuffer(size_t channel) const ;
    const std::vector<double>& getBuffer(size_t channel) const ;
    void setBuffer(size_t channel, std::vector<double> buffer);

    bool hasUpstreamBuffer(size_t channel) const ;
    const std::vector<double>& getUpstreamBuffer(size_t channel) const ;
    void setUpstreamModel(size_t inboundChannel, size_t outboundChannel, ComponentModel* outboundModel);
    void clearUpstreamModel(size_t channel);
    
    const ComponentDescriptor& getDescriptor() const ;
    ModulationModel* getModulationModel(ParameterType p) const ;
    const std::map<ParameterType, ModulationModel*>& getModulationModels() const ;

    const ParameterValue& getParameterValue(ParameterType p) const ;
    const std::pair<ParameterValue,ParameterValue>& getParameterRange(ParameterType p) const ;
    void setParameterValue(ParameterType p, ParameterValue v, bool block = false);
    void setParameterRange(ParameterType p, ParameterValue min, ParameterValue max, bool block = false);
    
    void setParameterToDefault(ParameterType p, bool block = false);
    void setRangeToDefault(ParameterType p, bool block = false);

private:
    bool validParam(ParameterType p) const ;

signals:
    void requestModelSync(int componentId);
    void requestBufferData(int componentId, size_t channel);

    void parameterValueChanged(ParameterType p, ParameterValue v) const ;
    void parameterRangeChanged(ParameterType p, ParameterValue min, ParameterValue max) const ;
    void collectionUpdated(const CollectionRequest& req);
    void modulationDepthChanged(int componentId, ParameterType p, double depth);
    void modulationStrategyChanged(int componentId, ParameterType p, ModulationStrategy strategy);
    void bufferUpdated(size_t channel);
    void upstreamBufferUpdated(size_t channel);
        
};

#endif // COMPONENT_MODEL_HPP_