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

#include "signal/SignalController.hpp"
#include "core/AudioEngine.hpp"
#include "core/AudioDevice.hpp"
#include "core/ComponentManager.hpp"
#include "requests/ConnectionRequest.hpp"
#include "types/ConnectionEndpoint.hpp"
#include "types/NodeType.hpp"

SignalController* SignalController::instance(){
    static SignalController controller ;
    return &controller ;
}

SignalController::SignalController(){
}

bool SignalController::handleConnection(const ConnectionRequest& req){
    const ConnectionEndpoint& inbound = req.inbound();
    const ConnectionEndpoint& outbound = req.outbound();

    // validate endpoints/components
    const bool inIsComponent = inbound.type() == NodeType::COMPONENT ;
    const bool outIsComponent = inbound.type() == NodeType::COMPONENT ;

    AudioIO* cIn = inIsComponent
        ? ComponentManager::instance()->getSignalComponent(outbound.id())
        : nullptr ;

    if ( inIsComponent && !cIn ){
        SPDLOG_ERROR("no component with id {} found. Cannot make connection", inbound.id());
        return false ;
    }

    AudioIO* cOut = outIsComponent
        ? ComponentManager::instance()->getSignalComponent(outbound.id())
        : nullptr ;

    if ( outIsComponent && !cOut ){
        SPDLOG_ERROR("no component with id {} found. Cannot make connection", outbound.id());
        return false ;
    }
    
    // component to component
    if ( cIn && cOut ){
        if ( req.remove() ){
            return disconnect(
                cOut, outbound.index().value(),
                cIn, inbound.index().value()
            );
        }
        return connect(
            cOut, outbound.index().value(),
            cIn, inbound.index().value()
        );
    }

    // sources
    if ( cIn ){
        if ( req.remove() ){
            return unregisterSource(
                cIn, inbound.index().value(), 
                outbound
            );    
        }
        return registerSource(
            cIn, inbound.index().value(), 
            outbound
        );
    }

    // sinks 
    if ( cOut ){
        if ( req.remove() ){
            return registerSink(
                cOut, outbound.index().value(),
                outbound
            );
        }
        return registerSink(
            cOut, outbound.index().value(),
            outbound
        );
    }

    // both are devices
    SPDLOG_ERROR("This application does not currently support device to device audio connections.");
    return false ;
}

bool SignalController::connect(
    AudioIO* outbound, size_t outboundIndex, 
    AudioIO* inbound, size_t inboundIndex
){
    if ( !outbound ) return false ;
    if ( !inbound )  return false ;

    if ( !inbound->connectInput(outbound,outboundIndex, inboundIndex) ){
        return false ;
    }

    if ( inbound->numOutputs() == 0 ){
        signalChain_.addPseudoSink(inbound, inboundIndex);
    }

    SPDLOG_DEBUG(
        "outbound signal {} index {} connected to inbound {} index {}. re-calculating signal chain.",
        fmt::ptr(outbound), outboundIndex, fmt::ptr(inbound), inboundIndex
    );
    signalChain_.calculateTopologicalOrder();

    return true ;
}

bool SignalController::disconnect(
    AudioIO* outbound, size_t outboundIndex, 
    AudioIO* inbound, size_t inboundIndex
){
    if ( !outbound ) return false ;
    if ( !inbound )  return false ;

    if ( inbound->disconnectInput(outbound, outboundIndex, inboundIndex) ){
        return false ;
    }

    if ( inbound->numOutputs() == 0 ){
        signalChain_.removePseudoSink(inbound, inboundIndex);
    }
    
    SPDLOG_DEBUG(
        "outbound signal {} index {} disconnected from inbound {} index {}. re-calculating signal chain",
        fmt::ptr(outbound), outboundIndex, fmt::ptr(inbound), inboundIndex
    );
    signalChain_.calculateTopologicalOrder();

    return true ;
}

bool SignalController::registerSink(
    AudioIO* outbound, size_t outboundIdx, 
    const ConnectionEndpoint& device
){
    if ( !signalChain_.addSink(outbound, outboundIdx, device) ){
        return false ;
    }
    signalChain_.calculateTopologicalOrder();
    return true ;
}

bool SignalController::unregisterSink(
    AudioIO* outbound, size_t outboundIdx, 
    const ConnectionEndpoint& device
){
    if ( !signalChain_.removeSink(outbound, outboundIdx, device) ){
        return false ;
    }
    signalChain_.calculateTopologicalOrder();
    return true ;
}

const AudioConnectionSet& SignalController::getSinks(const ConnectionEndpoint& device) const {
    return signalChain_.getSinks(device);
}

bool SignalController::registerSource(
    AudioIO* inbound, size_t inboundIdx, 
    const ConnectionEndpoint& device
){
    return signalChain_.addSource(inbound, inboundIdx, device);
}

bool SignalController::unregisterSource(
    AudioIO* inbound, size_t inboundIdx, 
    const ConnectionEndpoint& device
){
    return signalChain_.removeSource(inbound, inboundIdx, device);
}

const AudioConnectionSet& SignalController::getSources(const ConnectionEndpoint& device) const {
    return signalChain_.getSinks(device);
}

void SignalController::populateSources(const ConnectionEndpoint& device, double* data, size_t size){
    for ( auto& [c, ch] : getSources(device) ){
        c->
    }
}

void SignalController::processSignalChain(){
    const auto& chain = signalChain_.getSignalComponentChain();

    for ( AudioIO*  mod : chain ){
        mod->updateParameters();
        mod->calculateSample();
        mod->tick();
    }
}

void SignalController::clearBuffer(){
    for ( auto id : ComponentManager::instance()->getSignalints() ){
        auto m = ComponentManager::instance()->getSignalComponent(id);
        if ( m->isGenerative()){
            m->clearBuffer();
        }
    }
}

void SignalController::updateProcessingGraph(){
    signalChain_.calculateTopologicalOrder();
}

void SignalController::reset(){
    signalChain_.reset() ;
}