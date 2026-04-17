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

#include "core/ComponentManager.hpp"

ComponentManager::ComponentManager(MidiController* midiCtrl):
    midiController_(midiCtrl)
{}

BaseComponent* ComponentManager::getRaw(ComponentId id) const {
    auto it = components_.find(id);
    if ( it == components_.end() ) return nullptr ;
    return it->second.get() ;
}

BaseModule* ComponentManager::getModule(ComponentId id) const {
    if ( modules_.find(id) == modules_.end() ) return nullptr ;
    return dynamic_cast<BaseModule*>(getRaw(id));
}

const std::unordered_set<ComponentId>& ComponentManager::getModuleIds() const {
    return modules_ ;
}

BaseModulator* ComponentManager::getModulator(ComponentId id) const {
    if ( modulators_.find(id) == modulators_.end() ) return nullptr ;
    return dynamic_cast<BaseModulator*>(getRaw(id));
}

const std::unordered_set<ComponentId>& ComponentManager::getModulatorIds() const{
    return modulators_ ;
}

MidiEventHandler* ComponentManager::getMidiHandler(ComponentId id) const {
    if ( midiHandlers_.find(id) == midiHandlers_.end() ) return nullptr ;
    return dynamic_cast<MidiEventHandler*>(getRaw(id));
}

const std::unordered_set<ComponentId>& ComponentManager::getMidiHandlerIds() const {
    return midiHandlers_ ;
}

MidiEventListener* ComponentManager::getMidiListener(ComponentId id) const {
    if ( midiListeners_.find(id) == midiListeners_.end() ) return nullptr ;
    return dynamic_cast<MidiEventListener*>(getRaw(id));
}

const std::unordered_set<ComponentId>& ComponentManager::getMidiListenerIds() const {
    return midiListeners_ ;
}

void ComponentManager::remove(ComponentId id){
    midiHandlers_.erase(id);
    modules_.erase(id);
    modulators_.erase(id);

    components_.erase(id);
}

void ComponentManager::reset(){
    nextID_ = 0 ;
    components_.clear();
    midiHandlers_.clear();
    modulators_.clear();
    modules_.clear();
}

void ComponentManager::runParameterModulation(){
    for (auto it = components_.begin(); it != components_.end(); ++it){
        if ( !modules_.contains(it->first) ){
            it->second->updateParameters();
        }
    }
}

json ComponentManager::serializeComponent(BaseComponent* c) const {
    json output ;

    output["id"] = c->getId() ;
    output["name"] = ComponentRegistry::getComponentDescriptor(c->getType()).name ;

    // parameters
    output["parameters"] = c->getParameters()->toJson();

    // check modulation
    auto modulatableParameters = ComponentRegistry::getComponentDescriptor(
        c->getType()).modulatableParameters ;

    for ( auto p : modulatableParameters ){
        auto modulator = c->getParameterModulator(p);
        if ( modulator ){
            output["parameters"][GET_PARAMETER_TRAIT_MEMBER(p, name)]["modulatorId"] = modulator->getId();
        }
    }

    // get input signal component ids
    BaseModule* module = dynamic_cast<BaseModule*>(c);
    if ( module ){
        for ( size_t i = 0; i < module->getNumInputs(); ++i ){
            for ( const auto& conn : module->getInputs(i) ){
                output["signalInputs"][i].push_back(conn);
            }
        }
    }

    // get midi handler listeners
    MidiEventHandler* handler = dynamic_cast<MidiEventHandler*>(c);
    if ( handler ){
        for ( auto listener : handler->getListeners() ){
            auto listenerModule = dynamic_cast<BaseComponent*>(listener);
            if ( listenerModule ){
                output["midiListeners"].push_back(listenerModule->getId());
            }
        }
    }
    
    return output ;
}

json ComponentManager::serializeComponents() const {
    json output ;
    for ( auto& [id, c] : components_ ){
        output.push_back(serializeComponent(c.get()));
    }
    return output ;
}