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

#include "signal/SignalChain.hpp"

SignalChain::SignalChain():
    sinks_()
{}

void SignalChain::allocateOutputChannels(size_t numChannels){
    SPDLOG_DEBUG("allocating {} peripheral output channels", numChannels);
    for ( size_t i = 0 ; i < numChannels ; ++i ){
        if ( !sinks_.contains(i) ){
            sinks_[i];
        }
    }
}

const std::vector<AudioSignalComponent*>& SignalChain::getSignalComponentChain() const {
    return topologicalOrder_ ;
}

const SignalConnectionSet& SignalChain::getSinks(size_t channel) const {
    if ( !sinks_.contains(channel) ){
        SPDLOG_ERROR("channel {} requested but outbound nodes only has {} allocated", channel, sinks_.size());
    }
    assert(sinks_.contains(channel));
    return sinks_.at(channel);
}

void SignalChain::addSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx){
    if ( !outbound ){
        SPDLOG_WARN("Not adding a nullptr as a sink.");
        return ;
    }
    if ( outboundIdx > outbound->getNumOutputs() ){
        SPDLOG_WARN("outbound index out of bounds for component. Cannot add requested sink.");
        return ;
    }
    
    sinks_[inboundIdx].insert({outbound, outboundIdx});
}

void SignalChain::removeSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx){
    if ( !outbound || outboundIdx > outbound->getNumOutputs() ){
        SPDLOG_WARN("outbound index out of bounds for specified component. Cannot remove sink.");
        return ; 
    }

    sinks_[inboundIdx].erase({outbound, outboundIdx});
}

void SignalChain::addPseudoSink(AudioSignalComponent* component, size_t channel){
    if ( !component || channel > component->getNumInputs() ){
        SPDLOG_WARN("inbound index out of bounds for specified component. Cannot remove pseudo-sink.");
        return ;
    }

    SPDLOG_DEBUG("component with id {} added as pseudo-sink.", component->getId());
    pseudoSinks_.insert({component, channel});
}

void SignalChain::removePseudoSink(AudioSignalComponent* component, size_t channel){
    if ( !component || channel > component->getNumInputs() ){
        SPDLOG_WARN("inbound index out of bounds for specified component. Cannot add pseudo-sink.");
        return ;
    }

    SPDLOG_DEBUG("component with id {} is no longer a pseudo-sink.", component->getId());
    pseudoSinks_.erase({component, channel});
}

void SignalChain::calculateTopologicalOrder(){
    visited_.clear();
    topologicalOrder_.clear();
    
    // global post-order depth-first search
    for ( const auto& [channel, connections]: sinks_ ){
        for ( const auto conn : connections ){
            topologicalSort(conn.component, visited_, topologicalOrder_);
        }
    }

    for ( const auto& conn: pseudoSinks_ ){
        topologicalSort(conn.component, visited_, topologicalOrder_);
    }
}

void SignalChain::reset(){
    sinks_.clear();
    visited_.clear();
    topologicalOrder_.clear();
}

void SignalChain::topologicalSort(
    AudioSignalComponent* module, 
    std::unordered_set<AudioSignalComponent*>& visited,
    std::vector<AudioSignalComponent*>& result    
){
    if ( visited.count(module) ) return ;
    visited.insert(module);

    // Process stateful modulators in signal chain (e.g., Oscillator)
    for ( AudioSignalComponent* m : module->getModulationInputs() ){
        topologicalSort(m, visited, result);
    }

    // Now process normal signal chain
    for ( size_t i = 0; i < module->getNumInputs(); ++i ){
        for ( const auto& conn : module->getInputs(i)){
            topologicalSort(conn.component, visited, result);
        }
    }
    
    result.push_back(module); // post-order traversal (only insert once all inputs are  visited)
}