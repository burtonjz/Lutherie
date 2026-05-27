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

AudioSignalComponent* ComponentManager::getSignalComponent(ComponentId id) const {
    if ( audioSignals_.find(id) == audioSignals_.end() ) return nullptr ;
    return dynamic_cast<AudioSignalComponent*>(getRaw(id));
}

AudioBufferComponent* ComponentManager::getBufferComponent(ComponentId id) const {
    if ( audioBuffers_.find(id) == audioBuffers_.end() ) return nullptr ;
    return dynamic_cast<AudioBufferComponent*>(getRaw(id));
}

const std::unordered_set<ComponentId>& ComponentManager::getComponentIds() const {
    return allIds_ ;
}

const std::unordered_set<ComponentId>& ComponentManager::getSignalComponentIds() const {
    return audioSignals_ ;
}

ModulatorComponent* ComponentManager::getModulator(ComponentId id) const {
    if ( modulators_.find(id) == modulators_.end() ) return nullptr ;
    return dynamic_cast<ModulatorComponent*>(getRaw(id));
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

Analyzer* ComponentManager::getAnalyzer(ComponentId id) const {
    if ( analyzers_.find(id) == analyzers_.end() ) return nullptr ;
    return dynamic_cast<Analyzer*>(getRaw(id));
}

const std::unordered_set<ComponentId>& ComponentManager::getMidiListenerIds() const {
    return midiListeners_ ;
}

const std::unordered_set<ComponentId>& ComponentManager::getAnalyzerIds() const {
    return analyzers_ ;
}

void ComponentManager::remove(ComponentId id){
    allIds_.erase(id);
    midiHandlers_.erase(id);
    midiListeners_.erase(id);
    modulators_.erase(id);
    audioSignals_.erase(id);
    analyzers_.erase(id);
    components_.erase(id);
}

void ComponentManager::reset(){
    nextID_ = 0 ;
    allIds_.clear();
    midiHandlers_.clear();
    midiListeners_.clear();
    modulators_.clear();
    audioSignals_.clear();
    analyzers_.clear();
}

void ComponentManager::runParameterModulation(){
    for (auto it = components_.begin(); it != components_.end(); ++it){
        if ( !audioSignals_.contains(it->first) ){
            it->second->updateParameters();
        }
    }
}

void ComponentManager::runAnalyzers(){
    for ( auto id : analyzers_ ){
        auto a = getAnalyzer(id);
        a->flush();
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

    return output ;
}

json ComponentManager::serializeComponents() const {
    json output ;
    for ( auto& [id, c] : components_ ){
        output.push_back(serializeComponent(c.get()));
    }
    return output ;
}

void ComponentManager::getComponentConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const {
    getComponentSignalConnections(id, requests);
    getComponentMidiConnections(id, requests);
    getComponentModulationConnections(id, requests);
}

void ComponentManager::getComponentSignalConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const {
    SPDLOG_DEBUG("getting signal connections for component id = {}", id);

    AudioSignalComponent* module = getSignalComponent(id);
    Analyzer* analyzer = getAnalyzer(id);

    if ( !module && !analyzer ){
        SPDLOG_DEBUG("cannot get signal connections for component with id = {}. It is not an analyzer or module.", id);
        return ;
    }

    // Case 1: Analyzer
    if ( analyzer ){
        for ( const auto& [m, idx] : analyzer->getSources() ){
            ConnectionRequest req ;
            req.outboundID = m->getId();
            req.outboundIdx = idx ;
            req.outboundSocket = SocketType::SignalOutbound ;

            req.inboundID = id ;
            req.inboundIdx = 0 ;
            req.inboundSocket = SocketType::SignalInbound ;
            
            requests.push_back(req);
        }
        return ;
    } 
    
    // Case 2: signal component

    // check if module is connected to any analyzer
    for ( const auto& [a, idx] : module->getAnalyzers() ){
        if ( a ){
            ConnectionRequest req ;
            req.outboundID = id ;
            req.outboundIdx = idx ;
            req.outboundSocket = SocketType::SignalOutbound ;
            req.inboundID = a->getId() ;
            req.inboundIdx = 0 ;
            req.inboundSocket = SocketType::SignalInbound ;
            requests.push_back(req);
        }
    }

    // signal inputs
    for ( size_t i = 0; i < module->getNumInputs(); ++i ){
        for ( const auto& conn : module->getInputs(i) ){
            if ( !conn.component ) continue ;
            ConnectionRequest req ;
            req.inboundID = id ;
            req.inboundIdx = i ;
            req.inboundSocket = SocketType::SignalInbound ;
            req.outboundID = conn.component->getId() ;
            req.outboundIdx = conn.index ;
            req.outboundSocket = SocketType::SignalOutbound ;
            requests.push_back(req);
        }    
    }
    
    // signal outputs
    for ( size_t i = 0; i < module->getNumOutputs(); ++i ){
        for ( const auto& conn : module->getOutputs(i) ){
            if ( !conn.component ) continue ;
            ConnectionRequest req ;
            req.inboundID = conn.component->getId() ;
            req.inboundIdx = conn.index ;
            req.inboundSocket = SocketType::SignalInbound ;
            req.outboundID = id ;
            req.outboundIdx = i ;
            req.outboundSocket = SocketType::SignalOutbound ;
            requests.push_back(req);
        }
    }
    

    return ;
}


void ComponentManager::getComponentModulationConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const {
    SPDLOG_DEBUG("getting modulation connections for component id = {}", id);
    BaseComponent* module = getSignalComponent(id);
    ModulatorComponent* modulator = getModulator(id);

    // get all inbound parameter modulators
    if ( module ){
        auto d = ComponentRegistry::getComponentDescriptor(module->getType());
        for ( auto p : d.modulatableParameters ){
            ModulatorComponent* paramModulator = module->getParameterModulator(p);
            if ( paramModulator ){
                ConnectionRequest req ;
                req.inboundID = id ;
                req.inboundSocket = SocketType::ModulationInbound ;
                req.inboundParameter = p ;
                req.outboundID = paramModulator->getId() ;
                req.outboundSocket = SocketType::ModulationOutbound ;
                requests.push_back(req);
            }
        }
    }

    // if this component is also a modulator, add in what it is modulating
    if ( modulator ){
        for ( auto t : modulator->getModulationTargets() ){
            if ( t.component ){
                ConnectionRequest req ;
                req.inboundID = t.component->getId() ;
                req.inboundSocket = SocketType::ModulationInbound ;
                req.inboundParameter = t.param ;
                req.outboundID = id ;
                req.outboundSocket = SocketType::ModulationOutbound ;
                req.depthConnection = t.depth ;
                requests.push_back(req);
            }
        }
    }

    return ;
}


void ComponentManager::getComponentMidiConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const {
    SPDLOG_DEBUG("getting midi connections for component id = {}", id);
    BaseComponent* c = getRaw(id);

    if ( !c ) return ;

    MidiEventHandler* h = getMidiHandler(id);
    if ( h ){
        // also create a connection request for all listeners
        for ( auto listener : h->getListeners() ){
            if ( listener ){
                ConnectionRequest req ;
                req.inboundID = listener->getId();
                req.inboundSocket = SocketType::MidiInbound ;
                req.outboundID = id ;
                req.outboundSocket = SocketType::MidiOutbound ;
                requests.push_back(req);
            }
        }
    }

    // if it's a listener, create a connection request for all handlers
    MidiEventListener* listener = getMidiListener(id);
    if ( listener ){
        for ( auto handler : listener->getHandlers() ){
            if ( handler ){
                ConnectionRequest req ;
                req.inboundID = id ;
                req.inboundSocket = SocketType::MidiInbound ;
                ComponentId handlerId = handler->getId();
                if ( handlerId != -1 ){
                    req.outboundID =  handlerId ;
                }
                req.outboundSocket = SocketType::MidiOutbound ;
                requests.push_back(req);
            }
        }
    }
}

void ComponentManager::getComponentBufferConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const {
    AudioBufferComponent* component = getBufferComponent(id);
    if ( !component ) return ;

    for ( size_t i = 0; i < component->getNumInputs(); ++i ){
        for ( const auto& conn : component->getInputs(i) ){
            if ( !conn.component ) continue ;
            ConnectionRequest req ;
            req.inboundID = component->getId();
            req.inboundIdx = i ;
            req.inboundSocket = SocketType::BufferInbound ;
            req.outboundID = conn.component->getId();
            req.outboundIdx = conn.index ; 
            req.outboundSocket = SocketType::BufferOutbound ;
            requests.push_back(req);
        }
    }

    for ( size_t i = 0; i < component->getNumOutputs(); ++i ){
        for ( const auto& conn : component->getOutputs(i) ){
            if ( !conn.component ) continue ;
            ConnectionRequest req ;
            req.inboundID = conn.component->getId();
            req.inboundIdx = conn.index ; 
            req.inboundSocket = SocketType::BufferInbound ;
            req.outboundID = component->getId();
            req.outboundIdx = i ;
            req.outboundSocket = SocketType::BufferOutbound ;
            requests.push_back(req);
        }
    }
}