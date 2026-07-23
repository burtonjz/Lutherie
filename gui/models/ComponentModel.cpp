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


ComponentModel::ComponentModel(int id, ComponentType typ):
    id_(id),
    type_(typ)
{
    descriptor_ = ComponentRegistry::getComponentDescriptor(type_);

    for ( const auto& p : descriptor_.controllableParameters ){
        setParameterToDefault(p);
        setRangeToDefault(p);
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

    name_ = QString::fromStdString(descriptor_.name);
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

const QString& ComponentModel::getName() const {
    return name_ ;
}

void ComponentModel::setName(QString name){
    name_ = name ;
}

std::string ComponentModel::getFile() const {
    if ( file_.has_value() ) return file_.value() ;
    return "" ;
}

void ComponentModel::setFile(std::string name){
    file_ = name ;
}

bool ComponentModel::hasBuffer(size_t channel) const {
    return buffers_.contains(channel);
}   

const std::vector<double>& ComponentModel::getBuffer(size_t channel) const {
    if ( !hasBuffer(channel) ){
        throw std::runtime_error("buffer/channel not present in component model");
    }
    return buffers_.at(channel);
}

void ComponentModel::setBuffer(size_t channel, std::vector<double> buffer){
    buffers_[channel] = buffer ;
    emit bufferUpdated(channel);
}

bool ComponentModel::hasUpstreamBuffer(size_t channel) const {
    if ( !upstream_.contains(channel) ) return false ;

    auto [outbound, m] = upstream_.at(channel);
    return m && m->hasBuffer(outbound);
}

const std::vector<double>& ComponentModel::getUpstreamBuffer(size_t channel) const {
    if ( !hasUpstreamBuffer(channel) ){
        throw std::runtime_error("upstream buffer/channel not present in component model");
    }
    auto [outbound, m] = upstream_.at(channel);
    return m->getBuffer(outbound);
}

void ComponentModel::setUpstreamModel(size_t inboundChannel, size_t outboundChannel, ComponentModel* outboundModel){
    auto it = upstream_.find(inboundChannel);
    if ( it != upstream_.end() && it->second.second ){
        disconnect(
            it->second.second, &ComponentModel::bufferUpdated,
            this, nullptr
        );
    }
    SPDLOG_DEBUG(
        "setting upstream model for buffer reference. "
        "inbound channel {} references outbound buffer model with ID {} from channel {}",
        inboundChannel, outboundModel->getId(), outboundChannel
    );
    upstream_[inboundChannel] = std::pair<size_t, ComponentModel*>(outboundChannel, outboundModel);
    
    emit upstreamBufferUpdated(inboundChannel);
    connect(
        outboundModel, &ComponentModel::bufferUpdated,
        [this, inboundChannel, outboundChannel](size_t channel){
            if ( channel == outboundChannel ){
                emit upstreamBufferUpdated(inboundChannel);
            }
        }
    );
}

void ComponentModel::clearUpstreamModel(size_t inboundChannel){
    ComponentModel* upstreamModel = nullptr ;
    if ( upstream_.contains(inboundChannel) ){
        upstreamModel = upstream_.at(inboundChannel).second ;
        upstream_.erase(inboundChannel);
        emit upstreamBufferUpdated(inboundChannel);
    } else {
        SPDLOG_WARN(
            "received request to clear upstream model for channel {}, "
            "but no upstream model defined for that channel. Ignoring request.",
            inboundChannel
        );
        return ;
    }
    
    if ( upstreamModel ){
        disconnect(
            upstreamModel, &ComponentModel::bufferUpdated,
            this, &ComponentModel::upstreamBufferUpdated
        );
    }
}

const ComponentDescriptor& ComponentModel::getDescriptor() const {
    return descriptor_ ;
}

ModulationModel* ComponentModel::getModulationModel(ParameterType p) const {
    if ( ! modulations_.at(p) ) return nullptr ;
    return modulations_.at(p);
}

const std::map<ParameterType, ModulationModel*>& ComponentModel::getModulationModels() const {
    return modulations_ ;
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

const std::pair<ParameterValue,ParameterValue>& ComponentModel::getParameterRange(ParameterType p) const {
    if ( !validParam(p) ){
        throw std::logic_error(
            "FATAL: Invalid Parameter " + GET_PARAMETER_TRAIT_MEMBER(p, name)
            + " accessed in an unsupported Component Type (" + static_cast<char>(type_) 
            + "). This is a programming bug. "
        ); 
    }
    return paramRanges_.at(p);
}

void ComponentModel::setParameterValue(ParameterType p, ParameterValue v, bool block){
    if ( !validParam(p) ){
        SPDLOG_WARN("invalid parameter specified: {}. Cannot set parameter.", 
            GET_PARAMETER_TRAIT_MEMBER(p, name));
        return ;
    }
    parameters_[p] = v ;
    
    if ( !block ){
        emit parameterValueChanged(p, v);
    }
}

void ComponentModel::setParameterRange(ParameterType p, ParameterValue min, ParameterValue max, bool block){
    if ( !validParam(p) ){
        SPDLOG_WARN("invalid parameter specified: {}. Cannot set parameter range.", 
            GET_PARAMETER_TRAIT_MEMBER(p, name));
        return ;
    }
    paramRanges_[p] = {min, max};

    if ( !block ){
        emit parameterRangeChanged(p, min, max);
    }
}

void ComponentModel::setParameterToDefault(ParameterType p, bool block){
    if ( !validParam(p) ){
        SPDLOG_WARN("invalid parameter specified: {}. Cannot set parameter to default.", 
            GET_PARAMETER_TRAIT_MEMBER(p, name));
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
            SPDLOG_WARN("invalid parameter specified: {}. Cannot set parameter to default.", 
                GET_PARAMETER_TRAIT_MEMBER(p, name));
            break ;
    }

    if ( !block ){
        emit parameterValueChanged(p, getParameterValue(p) );
    }
}

void ComponentModel::setRangeToDefault(ParameterType p, bool block){
    if ( !validParam(p) ){
        SPDLOG_WARN("invalid parameter specified: {}. Cannot set parameter to default.", 
            GET_PARAMETER_TRAIT_MEMBER(p, name));
        return ;
    }
    
    switch(p){
        #define X(name) case ParameterType::name:                           \
            paramRanges_[p] = {                                             \
                static_cast<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>( \
                    GET_PARAMETER_TRAIT_MEMBER(p, minimum)),                \
                static_cast<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>( \
                    GET_PARAMETER_TRAIT_MEMBER(p, maximum))                 \
            } ; \
            break ;
        PARAMETER_TYPE_LIST
        #undef X
        default:
            SPDLOG_WARN("invalid parameter specified: {}. Cannot set parameter range to default.", 
                GET_PARAMETER_TRAIT_MEMBER(p, name));
            break ;
    }

    if ( !block ){
        const auto& [min,max] = getParameterRange(p);
        emit parameterRangeChanged(p, min, max);
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

