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
#include "requests/CollectionRequest.hpp"
#include "api/ControlApiHandler.hpp"
#include "midi/MidiControlRouter.hpp"

ComponentManager* ComponentManager::instance(){
    static ComponentManager manager ;
    return &manager ;
}

ComponentManager::ComponentManager()
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

AudioProbe* ComponentManager::getAudioProbe(ComponentId id) const {
    if ( audioProbes_.find(id) == audioProbes_.end() ) return nullptr ;
    return dynamic_cast<AudioProbe*>(getRaw(id));
}

FileComponent* ComponentManager::getFileComponent(ComponentId id) const {
    if ( fileComponents_.find(id) == fileComponents_.end() ) return nullptr ;
    return dynamic_cast<FileComponent*>(getRaw(id));
}

const std::unordered_set<ComponentId>& ComponentManager::getMidiListenerIds() const {
    return midiListeners_ ;
}

const std::unordered_set<ComponentId>& ComponentManager::getAudioProbeIds() const {
    return audioProbes_ ;
}

const std::unordered_set<ComponentId>& ComponentManager::getFileComponentIds() const {
    return fileComponents_ ;
}

void ComponentManager::remove(ComponentId id){
    auto* component = getRaw(id);
    if ( !component ) return ;

    // unregister from other objects if necessary
    if ( MidiEventHandler* h = getMidiHandler(id) ){
        MidiController::instance()->removeHandler(h);
    }

    // clear views
    allIds_.erase(id);
    midiHandlers_.erase(id);
    midiListeners_.erase(id);
    modulators_.erase(id);
    audioSignals_.erase(id);
    audioProbes_.erase(id);

    components_.erase(id); // deletes unique ptr & frees memory
}

void ComponentManager::reset(){
    nextID_ = 0 ;
    allIds_.clear();
    midiHandlers_.clear();
    midiListeners_.clear();
    modulators_.clear();
    audioSignals_.clear();
    audioProbes_.clear();

    components_.clear();
}

void ComponentManager::runParameterModulation(){
    for (auto it = components_.begin(); it != components_.end(); ++it){
        if ( !audioSignals_.contains(it->first) ){
            it->second->updateParameters();
        }
    }
}

void ComponentManager::flushAudioProbes(){
    for ( auto id : audioProbes_ ){
        auto a = getAudioProbe(id);
        a->flush();
    }
}

json ComponentManager::serializeComponent(BaseComponent* c) const {
    json output ;

    output["id"] = c->getId() ;
    output["name"] = ComponentRegistry::getComponentDescriptor(c->getType()).name ;

    // parameters / collections
    c->getParameters()->toJson(output);

    auto cd = ComponentRegistry::getComponentDescriptor(c->getType());

    // collections need to be reconfigured into a GET_ALL CollectionRequest to be readable by client
    if ( cd.hasCollection() ){
        CollectionRequest req = {
            .action = CollectionAction::GET_ALL,
            .componentId = c->getId(),
            .value = std::nullopt,
            .index = std::nullopt
        };
        json r = req ;
        output["collection"] = ControlApiHandler::instance()
            ->parseCollectionRequest(r);
    }

    // modulation
    for ( auto p : cd.modulatableParameters ){
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
    getComponentBufferConnections(id, requests);
}

void ComponentManager::getComponentSignalConnections(ComponentId id, std::vector<ConnectionRequest>& requests) const {
    SPDLOG_DEBUG("getting signal connections for component id = {}", id);

    AudioSignalComponent* component = getSignalComponent(id);

    if ( !component ){
        SPDLOG_DEBUG("cannot get signal connections for component with id = {}. It is not audio signal component.", id);
        return ;
    }

    // signal inputs
    for ( size_t i = 0; i < component->getNumInputs(); ++i ){
        for ( const auto& conn : component->getInputs(i) ){
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
    for ( size_t i = 0; i < component->getNumOutputs(); ++i ){
        for ( const auto& conn : component->getOutputs(i) ){
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