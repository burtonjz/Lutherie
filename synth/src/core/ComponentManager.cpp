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
#include <optional>

ComponentManager* ComponentManager::instance(){
    static ComponentManager manager ;
    return &manager ;
}

ComponentManager::ComponentManager()
{}

BaseComponent* ComponentManager::getRaw(int id) const {
    auto it = components_.find(id);
    if ( it == components_.end() ) return nullptr ;
    return it->second.get() ;
}

AudioIO* ComponentManager::getSignalComponent(int id) const {
    if ( audioSignals_.find(id) == audioSignals_.end() ) return nullptr ;
    return dynamic_cast<AudioIO*>(getRaw(id));
}

BufferIO* ComponentManager::getBufferComponent(int id) const {
    if ( audioBuffers_.find(id) == audioBuffers_.end() ) return nullptr ;
    return dynamic_cast<BufferIO*>(getRaw(id));
}

const std::unordered_set<int>& ComponentManager::getints() const {
    return allIds_ ;
}

const std::unordered_set<int>& ComponentManager::getSignalints() const {
    return audioSignals_ ;
}

ModulatorComponent* ComponentManager::getModulator(int id) const {
    if ( modulators_.find(id) == modulators_.end() ) return nullptr ;
    return dynamic_cast<ModulatorComponent*>(getRaw(id));
}

const std::unordered_set<int>& ComponentManager::getModulatorIds() const{
    return modulators_ ;
}

MidiHandler* ComponentManager::getMidiHandler(int id) const {
    if ( midiHandlers_.find(id) == midiHandlers_.end() ) return nullptr ;
    return dynamic_cast<MidiHandler*>(getRaw(id));
}

const std::unordered_set<int>& ComponentManager::getMidiHandlerIds() const {
    return midiHandlers_ ;
}

MidiListener* ComponentManager::getMidiListener(int id) const {
    if ( midiListeners_.find(id) == midiListeners_.end() ) return nullptr ;
    return dynamic_cast<MidiListener*>(getRaw(id));
}

AudioProbe* ComponentManager::getAudioProbe(int id) const {
    if ( audioProbes_.find(id) == audioProbes_.end() ) return nullptr ;
    return dynamic_cast<AudioProbe*>(getRaw(id));
}

FileComponent* ComponentManager::getFileComponent(int id) const {
    if ( fileComponents_.find(id) == fileComponents_.end() ) return nullptr ;
    return dynamic_cast<FileComponent*>(getRaw(id));
}

const std::unordered_set<int>& ComponentManager::getMidiListenerIds() const {
    return midiListeners_ ;
}

const std::unordered_set<int>& ComponentManager::getAudioProbeIds() const {
    return audioProbes_ ;
}

const std::unordered_set<int>& ComponentManager::getFileints() const {
    return fileComponents_ ;
}

void ComponentManager::remove(int id){
    auto* component = getRaw(id);
    if ( !component ) return ;

    // unregister from other objects if necessary
    if ( MidiHandler* h = getMidiHandler(id) ){
        MidiController::instance()->removeHandler(h);
    }

    // clear views
    allIds_.erase(id);
    midiHandlers_.erase(id);
    midiListeners_.erase(id);
    modulators_.erase(id);
    audioSignals_.erase(id);
    audioProbes_.erase(id);

    SPDLOG_DEBUG("deleting component id {} at address {}.", id, fmt::ptr(component));
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

void ComponentManager::getComponentConnections(int id, std::vector<ConnectionRequest>& requests) const {
    getComponentAudioConnections(id, requests);
    getComponentMidiConnections(id, requests);
    getComponentModulationConnections(id, requests);
    getComponentBufferConnections(id, requests);
}

void ComponentManager::getComponentAudioConnections(int id, std::vector<ConnectionRequest>& requests) const {
    SPDLOG_DEBUG("getting signal connections for component id = {}", id);

    AudioIO* component = getSignalComponent(id);

    if ( !component ){
        SPDLOG_DEBUG("cannot get signal connections for component with id = {}. It is not audio signal component.", id);
        return ;
    }

    // signal inputs
    for ( size_t i = 0; i < component->numInputs(); ++i ){
        ConnectionEndpoint inbound = ConnectionEndpoint::create(
            NodeType::COMPONENT, SocketType::SignalInbound,
            id, i
        );
        for ( const auto& conn : component->getInputs(i) ){
            if ( !conn.component ) continue ;
            ConnectionEndpoint outbound = ConnectionEndpoint::create(
                NodeType::COMPONENT, SocketType::SignalOutbound, 
                conn.component->getId(), conn.index
            );
            requests.push_back(ConnectionRequest(outbound, inbound));
        }    
    }
    
    // signal outputs
    for ( size_t i = 0; i < component->numOutputs(); ++i ){
        ConnectionEndpoint outbound = ConnectionEndpoint::create(
            NodeType::COMPONENT, SocketType::SignalOutbound, 
            id, i
        );
        for ( const auto& conn : component->getOutputs(i) ){
            if ( !conn.component ) continue ;
            ConnectionEndpoint inbound = ConnectionEndpoint::create(
                NodeType::COMPONENT, SocketType::SignalInbound, 
                conn.component->getId(), conn.index 
            );
            requests.push_back(ConnectionRequest(outbound, inbound));
        }
    }
    

    return ;
}


void ComponentManager::getComponentModulationConnections(int id, std::vector<ConnectionRequest>& requests) const {
    SPDLOG_DEBUG("getting modulation connections for component id = {}", id);
    BaseComponent* module = getSignalComponent(id);
    ModulatorComponent* modulator = getModulator(id);

    // get all inbound parameter modulators
    if ( module ){
        auto d = ComponentRegistry::getComponentDescriptor(module->getType());
        for ( auto p : d.modulatableParameters ){
            ModulatorComponent* paramModulator = module->getParameterModulator(p);
            if ( paramModulator ){
                ConnectionEndpoint outbound = ConnectionEndpoint::create(
                    NodeType::COMPONENT, SocketType::ModulationOutbound, 
                    paramModulator->getId()
                );
                ConnectionEndpoint inbound = ConnectionEndpoint::create(
                    NodeType::COMPONENT, SocketType::ModulationInbound, 
                    id, std::nullopt, p
                );
                requests.push_back(ConnectionRequest(outbound, inbound));
            }

            ModulatorComponent* depthModulator = module->getParameterDepthModulator(p);
            if ( depthModulator ){
                ConnectionEndpoint outbound = ConnectionEndpoint::create(
                    NodeType::COMPONENT, SocketType::ModulationOutbound, 
                    depthModulator->getId()
                );
                ConnectionEndpoint inbound = ConnectionEndpoint::create(
                    NodeType::COMPONENT, SocketType::ModulationInbound, 
                    id, std::nullopt, p
                );
                requests.push_back(ConnectionRequest(
                    outbound, inbound, 
                    false, true
                ));
            }
        }
    }

    // if this component is also a modulator, add in what it is modulating
    if ( modulator ){
        for ( auto t : modulator->getModulationTargets() ){
            if ( t.component ){
                ConnectionEndpoint outbound = ConnectionEndpoint::create(
                    NodeType::COMPONENT, SocketType::ModulationOutbound,
                    id
                );
                ConnectionEndpoint inbound = ConnectionEndpoint::create(
                    NodeType::COMPONENT, SocketType::ModulationInbound, 
                    t.component->getId(), std::nullopt, t.param
                );
                requests.push_back(ConnectionRequest(
                    outbound, inbound, 
                    false, t.depth
                ));
            }
        }
    }

    return ;
}


void ComponentManager::getComponentMidiConnections(int id, std::vector<ConnectionRequest>& requests) const {
    SPDLOG_DEBUG("getting midi connections for component id = {}", id);
    BaseComponent* c = getRaw(id);

    if ( !c ) return ;

    MidiHandler* h = getMidiHandler(id);
    if ( h ){
        ConnectionEndpoint outbound = ConnectionEndpoint::create(
            NodeType::COMPONENT, SocketType::MidiOutbound, 
            id
        );
        for ( auto listener : h->getListeners() ){
            if ( listener ){
                ConnectionEndpoint inbound = ConnectionEndpoint::create(
                    NodeType::COMPONENT, SocketType::MidiInbound, 
                    listener->getId()
                );
                requests.push_back(ConnectionRequest(
                    outbound, inbound
                ));
            }
        }
    }

    // if it's a listener, create a connection request for all handlers
    MidiListener* listener = getMidiListener(id);
    if ( listener ){
        ConnectionEndpoint inbound = ConnectionEndpoint::create(
            NodeType::COMPONENT, SocketType::MidiInbound, 
            id
        );
        for ( auto handler : listener->getHandlers() ){
            if ( handler ){
                ConnectionEndpoint outbound = ConnectionEndpoint::create(
                    NodeType::COMPONENT, SocketType::MidiOutbound, 
                    handler->getId()
                );
                requests.push_back(ConnectionRequest(
                    outbound, inbound
                ));
            }
        }
    }
}

void ComponentManager::getComponentBufferConnections(int id, std::vector<ConnectionRequest>& requests) const {
    BufferIO* component = getBufferComponent(id);
    if ( !component ) return ;

    for ( size_t i = 0; i < component->numInputs(); ++i ){
        ConnectionEndpoint inbound = ConnectionEndpoint::create(
            NodeType::COMPONENT, SocketType::BufferInbound,
            component->getId(), i
        );
        for ( const auto& conn : component->getInputs(i) ){
            if ( !conn.component ) continue ;
            ConnectionEndpoint outbound = ConnectionEndpoint::create(
                NodeType::COMPONENT, SocketType::BufferOutbound, 
                conn.index, conn.component->getId()
            );
            requests.push_back(ConnectionRequest(
                outbound, inbound
            ));
        }
    }

    for ( size_t i = 0; i < component->numOutputs(); ++i ){
        ConnectionEndpoint outbound = ConnectionEndpoint::create(
            NodeType::COMPONENT, SocketType::BufferOutbound,
            component->getId(), i
        );
        for ( const auto& conn : component->getOutputs(i) ){
            if ( !conn.component ) continue ;
            ConnectionEndpoint inbound = ConnectionEndpoint::create(
                NodeType::COMPONENT, SocketType::BufferInbound,
                conn.component->getId(), conn.index
            );
            requests.push_back(ConnectionRequest(
                outbound, inbound
            ));
        }
    }
}