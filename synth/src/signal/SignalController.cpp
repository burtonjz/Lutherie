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
#include "core/ComponentManager.hpp"

SignalController* SignalController::instance(){
    static SignalController controller ;
    return &controller ;
}

SignalController::SignalController(){
    setNumChannels(1); // prime output channels
}

size_t SignalController::getNumChannels() const {
    return numChannels_ ;
}

void SignalController::setNumChannels(size_t numChannels){
    numChannels_ = numChannels ;
    signalChain_.allocateOutputChannels(numChannels);
    outputs_.resize(numChannels);
}

void SignalController::connect(
    AudioSignalComponent* outbound, size_t outboundIndex, 
    AudioSignalComponent* inbound, size_t inboundIndex
){
    if (!outbound) return ;
    if (!inbound) return ;
    inbound->connectInput(outbound,outboundIndex, inboundIndex);
    if ( inbound->getNumOutputs() == 0 ){
        signalChain_.addPseudoSink(inbound, inboundIndex);
    }
    signalChain_.calculateTopologicalOrder();
}

void SignalController::disconnect(
    AudioSignalComponent* outbound, size_t outboundIndex, 
    AudioSignalComponent* inbound, size_t inboundIndex
){
    if (!outbound) return ;
    if (!inbound) return ;
    inbound->disconnectInput(outbound, outboundIndex, inboundIndex);
    if ( inbound->getNumOutputs() == 0 ){
        signalChain_.removePseudoSink(inbound, inboundIndex);
    }
    signalChain_.calculateTopologicalOrder();
}

void SignalController::registerSink(
    AudioSignalComponent* outbound, size_t outboundIdx, 
    size_t inboundIdx
){
    signalChain_.addSink(outbound, outboundIdx, inboundIdx);
    signalChain_.calculateTopologicalOrder();
}

void SignalController::unregisterSink(
    AudioSignalComponent* outbound, size_t outboundIdx, 
    size_t inboundIdx
){
    signalChain_.removeSink(outbound, outboundIdx, inboundIdx);
    signalChain_.calculateTopologicalOrder();
}

const SignalConnectionSet& SignalController::getSinks(size_t channel) const {
    return signalChain_.getSinks(channel) ;
}

std::pair<double*, size_t> SignalController::processFrame(){
    const auto& chain = signalChain_.getSignalComponentChain();

    // process modules in chain order
    for ( AudioSignalComponent*  mod : chain ){
        mod->updateParameters();
        mod->calculateSample();
        mod->tick();
    }

    // sum up sinks
    for ( size_t channel = 0 ; channel < numChannels_ ; ++channel ){
        outputs_[channel] = 0.0 ;
        for ( const auto& conn : signalChain_.getSinks(channel) ){
            outputs_[channel] += conn.component->getLastSample(conn.index);
        }
    }

    return {outputs_.data(), outputs_.size()} ;
}

void SignalController::clearBuffer(){
    for ( auto id : ComponentManager::instance()->getSignalComponentIds() ){
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