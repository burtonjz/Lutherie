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

#include "models/ComponentModel.hpp"
#include "meta/ComponentRegistry.hpp"

#include <QDebug>


ComponentModel::ComponentModel(int id, ComponentType typ):
    id_(id),
    type_(typ)
{
    descriptor_ = ComponentRegistry::getComponentDescriptor(type_);

    for ( const auto& p : descriptor_.controllableParameters ){
        setParameterToDefault(p);
    }

    for ( const auto& p : descriptor_.modulatableParameters ){
        modulations_[p] = new ModulationModel(id_, p);

        connect(
            modulations_.at(p), &ModulationModel::modulationStrategyChanged,
            this, &ComponentModel::modulationStrategyChanged
        );

        connect(
            modulations_.at(p), &ModulationModel::modulationDepthChanged,
            this, &ComponentModel::modulationDepthChanged
        );
    }
}

ComponentModel::~ComponentModel(){
    for ( auto& [k,v] : modulations_ ){
        v->deleteLater();
    }
}

int ComponentModel::getId() const {
    return id_ ;
}

ComponentType ComponentModel::getType() const {
    return type_ ;
}

const ComponentDescriptor& ComponentModel::getDescriptor() const {
    return descriptor_ ;
}

ModulationModel* ComponentModel::getModulationModel(ParameterType p) const {
    if ( ! modulations_.at(p) ) return nullptr ;
    return modulations_.at(p);
}

const ParameterValue& ComponentModel::getParameterValue(ParameterType p) const {
    if ( !validParam(p) ){
        throw std::logic_error(
            "FATAL: Invalid Parameter " + GET_PARAMETER_TRAIT_MEMBER(p, name)
            + " accessed in an unsupported Component Type (" + static_cast<char>(type_) 
            + "). This is a programming bug. "
        ); 
    }
    return parameters_.at(p) ;
}

void ComponentModel::setParameterValue(ParameterType p, ParameterValue v, bool block){
    if ( !validParam(p) ){
        qWarning() << "invalid parameter specified: " << GET_PARAMETER_TRAIT_MEMBER(p, name);
        return ;
    }
    parameters_[p] = v ;
    
    if ( !block ){
        emit parameterValueChanged(p, v);
    }
}

void ComponentModel::setParameterToDefault(ParameterType p, bool block){
    if ( !validParam(p) ){
        qWarning() << "invalid parameter specified: " << GET_PARAMETER_TRAIT_MEMBER(p, name);
        return ;
    }
    
    switch(p){
        #define X(name) case ParameterType::name: \
            parameters_[p] = static_cast<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>( \
                GET_PARAMETER_TRAIT_MEMBER(p, defaultValue) \
            ); \
            break ;
        PARAMETER_TYPE_LIST
        #undef X
        default:
            qWarning() << "invalid parameter specified: " << GET_PARAMETER_TRAIT_MEMBER(p, name);
            break ;
    }

    if ( !block ){
        emit parameterValueChanged(p, getParameterValue(p) );
    }
}

bool ComponentModel::validParam(ParameterType p) const {
    auto it = std::find(
        descriptor_.controllableParameters.begin(), 
        descriptor_.controllableParameters.end(),
        p
    );
    return it != descriptor_.controllableParameters.end();
}

