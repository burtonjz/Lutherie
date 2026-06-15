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

#include "core/BaseComponent.hpp"
#include "core/AudioSignalComponent.hpp"
#include "core/ModulatorComponent.hpp" 
#include "params/ParameterMap.hpp"
#include "api/ApiHandler.hpp"
#include <unordered_set>


BaseComponent::BaseComponent(ComponentId id, ComponentType type):
    id_(id),
    type_(type),
    parameters_(new ParameterMap)
{}

BaseComponent::~BaseComponent(){
    delete parameters_ ;
};

std::unordered_set<AudioSignalComponent*>& BaseComponent::getModulationInputs(){
    return modulationModules_ ;
}

void BaseComponent::setParameterModulation(ParameterType p, ModulatorComponent* m, ModulationData d){
    if ( ! parameters_ ) return ;

    // if the modulator is stateful (also a module), track it for signal chain
    AudioSignalComponent* module = dynamic_cast<AudioSignalComponent*>(m);
    if ( module ){
        modulationModules_.insert(module);
    }
    
    // update reciprocol tracking
    m->addModulationTarget({.component = this, .param = p});

    onSetParameterModulation(p,m,d);
}

void BaseComponent::setParameterDepthModulation(ParameterType p, ModulatorComponent* m, ModulationData d){
    if ( ! parameters_ ) return ;

    // if the modulator is stateful (also a module), track it for signal chain
    AudioSignalComponent* module = dynamic_cast<AudioSignalComponent*>(m);
    if ( module ){
        modulationModules_.insert(module);
    }
    
    // update reciprocol tracking
    m->addModulationTarget({.component = this, .param = p, .depth = true});

    onSetParameterDepthModulation(p,m,d);
} 

void BaseComponent::removeParameterModulation(ParameterType p){
    if ( ! parameters_ ) return ;

    // if the modulator is stateful (also a module), remove tracking
    ModulatorComponent* modulator = getParameterModulator(p);
    if ( AudioSignalComponent* module = dynamic_cast<AudioSignalComponent*>(modulator) ){
        modulationModules_.erase(module);
    }

    // update reciprocol tracking
    if ( modulator ){
        modulator->removeModulationTarget({this,p});
    }
    
    onRemoveParameterModulation(p);
}

void BaseComponent::removeParameterDepthModulation(ParameterType p){
    if ( ! parameters_ ) return ;

    // if the modulator is stateful (also a module), remove tracking
    ModulatorComponent* modulator = getParameterDepthModulator(p);
    if ( AudioSignalComponent* module = dynamic_cast<AudioSignalComponent*>(modulator) ){
        modulationModules_.erase(module);
    }

    // update reciprocol tracking
    if ( modulator ){
        modulator->removeModulationTarget({this,p});
    }
    
    onRemoveParameterDepthModulation(p);
}

ModulatorComponent* BaseComponent::getParameterModulator(ParameterType p) const {
    return parameters_->getParameter(p)->getModulator();
}

ModulatorComponent* BaseComponent::getParameterDepthModulator(ParameterType p) const {
    return parameters_->getParameter(p)->getDepth()->getModulator();
}

double BaseComponent::getParameterDepth(ParameterType p) const {
    if ( ! parameters_ ) throw std::runtime_error("no parameters pointer defined!");

    auto d = parameters_->getParameter(p)->getDepth();

    if ( !d ) throw std::runtime_error("depth parameter does not exist!");
    return d->getInstantaneousValue();
}

void BaseComponent::setParameterDepth(ParameterType p, double depth){
    if ( ! parameters_ ) return ;

    auto d = parameters_->getParameter(p)->getDepth();

    if ( !d ) return ;
    d->setValue(depth);
}

ModulationStrategy BaseComponent::getParameterModulationStrategy(ParameterType p) const {
    if ( ! parameters_ ) throw std::runtime_error("no parameters pointer defined!");

    auto param = parameters_->getParameter(p);

    if ( ! param ) throw std::runtime_error("no parameters pointer defined!");

    return param->getModulationStrategy();
}

void BaseComponent::setParameterModulationStrategy(ParameterType p, ModulationStrategy strat){
    if ( ! parameters_ ) return ;

    auto param = parameters_->getParameter(p);

    if ( ! param ) return ;

    param->setModulationStrategy(strat);
}

void BaseComponent::updateParameters(){
    if (parameters_) parameters_->modulate() ;
}

void BaseComponent::onSetParameterModulation(ParameterType p, ModulatorComponent* m, ModulationData d ){
    if ( d.isEmpty() && m ){
        auto required = m->getRequiredModulationParameters();
        for ( auto mp : required ){
            d.set(mp,0.0f);
        }
    }
    parameters_->getParameter(p)->setModulation(m,d);
}

void BaseComponent::onRemoveParameterModulation(ParameterType p){
    parameters_->getParameter(p)->removeModulation();
}

void BaseComponent::onSetParameterDepthModulation(ParameterType p, ModulatorComponent* m, ModulationData d ){
    if ( d.isEmpty() && m ){
        auto required = m->getRequiredModulationParameters();
        for ( auto mp : required ){
            d.set(mp,0.0f);
        }
    }
    parameters_->getParameter(p)->getDepth()->setModulation(m,d);
}

void BaseComponent::onRemoveParameterDepthModulation(ParameterType p){
    parameters_->getParameter(p)->getDepth()->removeModulation();
}

void BaseComponent::triggerComponentSync() const {
    // notify clients
    auto* api = ApiHandler::instance();
    json j ;
    j["action"] = "sync_component" ;
    j["componentId"] = getId();
    for ( const auto& sock : api->getOpenClientSockets() ){
        ApiHandler::instance()->handleClientMessage(sock,j.dump());
    }
}