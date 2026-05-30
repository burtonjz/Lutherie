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

#ifndef __SIGNAL_CONTROLLER_HPP_
#define __SIGNAL_CONTROLLER_HPP_

#include "core/AudioSignalComponent.hpp"
#include "core/ComponentManager.hpp"
#include "signal/SignalChain.hpp"

class SignalController {
private:
    ComponentManager* components_ ;
    SignalChain signalChain_ ;
    size_t numChannels_ ;
    std::vector<double> outputs_ ;

public:
    SignalController(ComponentManager* components):
        components_(components)
    {
        setNumChannels(1); // prime output channels
    }

    size_t getNumChannels() const {
        return numChannels_ ;
    }

    void setNumChannels(size_t numChannels){
        numChannels_ = numChannels ;
        signalChain_.allocateOutputChannels(numChannels);
        outputs_.resize(numChannels);
    }

    // signal chain functions
    void connect(AudioSignalComponent* from, size_t fromIndex, AudioSignalComponent* to, size_t toIndex){
        if (!from) return ;
        if (!to) return ;
        to->connectInput(from,fromIndex, toIndex);
        signalChain_.calculateTopologicalOrder();
    }

    void disconnect(AudioSignalComponent* from, size_t fromIndex, AudioSignalComponent* to, size_t toIndex){
        if (!from) return ;
        if (!to) return ;
        to->disconnectInput(from, fromIndex, toIndex);
        signalChain_.calculateTopologicalOrder();
    }

    void registerSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx){
        signalChain_.addSink(outbound, outboundIdx, inboundIdx);
        signalChain_.calculateTopologicalOrder();
    }

    void unregisterSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx){
        signalChain_.removeSink(outbound, outboundIdx, inboundIdx);
        signalChain_.calculateTopologicalOrder();
    }

    const std::unordered_set<SignalConnection, ConnectionHash>& getSinks(size_t channel) const {
        return signalChain_.getSinks(channel) ;
    }

    std::pair<double*, size_t> processFrame(){
        auto chain = signalChain_.getSignalComponentChain();

        // process modules in chain order
        for (AudioSignalComponent*  mod : chain){
            mod->updateParameters();
            mod->calculateSample();
            mod->tick();
        }

        // sum up sinks
        for (size_t channel = 0 ; channel < numChannels_ ; ++channel ){
            outputs_[channel] = 0.0 ;
            for ( const auto& conn : signalChain_.getSinks(channel) ){
                outputs_[channel] += conn.component->getLastSample(conn.index);
            }
        }

        return {outputs_.data(), outputs_.size()} ;
    }

    void clearBuffer(){
        for ( auto id : components_->getSignalComponentIds() ){
            auto m = components_->getSignalComponent(id);
            if ( m->isGenerative()){
                m->clearBuffer();
            }
        }
    }

    void updateProcessingGraph(){
        signalChain_.calculateTopologicalOrder();
    }

    void reset(){
        signalChain_.reset() ;
    }
};

#endif // __SIGNAL_CONTROLLER_HPP_