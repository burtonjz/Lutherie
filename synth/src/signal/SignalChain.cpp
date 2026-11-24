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
#include "core/AudioDevice.hpp"
#include "types/ConnectionEndpoint.hpp"
#include <stdexcept>

SignalChain::SignalChain():
    sinks_()
{}

const std::vector<AudioIO*>& SignalChain::getSignalComponentChain() const {
    return topologicalOrder_ ;
}

const AudioConnectionSet& SignalChain::getSinks(const ConnectionEndpoint& device) const {
    if ( !sinks_.contains(device) ){
        auto msg = fmt::format("device endpoint '{}' is not registered in signal chain.", device.toString());
        SPDLOG_ERROR(msg);
        throw std::runtime_error(msg);
    }
    return sinks_.at(device);
}

bool SignalChain::addSink(AudioIO* component, size_t channel, const ConnectionEndpoint& device){
    if ( !component ){
        SPDLOG_WARN("Ignoring nullptr component.");
        return false ;
    }
    if ( channel > component->numOutputs() ){
        SPDLOG_ERROR("channel out of bounds for component.");
        return false ;
    }
    
    sinks_[device].insert({component, channel});
    return true ;
}

bool SignalChain::removeSink(AudioIO* component, size_t channel, const ConnectionEndpoint& device){
    if ( !component ){
        SPDLOG_WARN("Ignoring nullptr component.");
        return false ;
    }
    if ( channel > component->numOutputs() ){
        SPDLOG_ERROR("channel out of bounds for component.");
        return false ;
    }

    sinks_[device].erase({component, channel});
    return true ;
}

bool SignalChain::addPseudoSink(AudioIO* component, size_t channel){
    if ( !component ){
        SPDLOG_WARN("Ignoring nullptr component.");
        return false ;
    }
    if ( channel > component->numOutputs() ){
        SPDLOG_ERROR("channel out of bounds for component.");
        return false ;
    }

    SPDLOG_DEBUG("component with id {} added as pseudo-sink.", component->getId());
    pseudoSinks_.insert({component, channel});
    return true ;
}

bool SignalChain::removePseudoSink(AudioIO* component, size_t channel){
    if ( !component ){
        SPDLOG_WARN("Ignoring nullptr component.");
        return false ;
    }
    if ( channel > component->numOutputs() ){
        SPDLOG_ERROR("channel out of bounds for component.");
        return false ;
    }

    SPDLOG_DEBUG("component with id {} is no longer a pseudo-sink.", component->getId());
    pseudoSinks_.erase({component, channel});
    return true ;
}

const AudioConnectionSet& SignalChain::getSources(const ConnectionEndpoint& device) const {
    if ( !sources_.contains(device) ){
        auto msg = fmt::format("device endpoint '{}' is not registered in signal chain.", device.toString());
        SPDLOG_ERROR(msg);
        throw std::runtime_error(msg);
    }
    return sources_.at(device);
}

bool SignalChain::addSource(AudioIO* component, size_t channel, const ConnectionEndpoint& device){
    if ( !component ){
        SPDLOG_WARN("Ignoring nullptr component.");
        return false ;
    }
    if ( channel > component->numOutputs() ){
        SPDLOG_ERROR("channel out of bounds for component.");
        return false ;
    }
    
    sources_[device].insert({component, channel});
    return true ;
}

bool SignalChain::removeSource(AudioIO* component, size_t channel, const ConnectionEndpoint& device){
    if ( !component ){
        SPDLOG_WARN("Ignoring nullptr component.");
        return false ;
    }
    if ( channel > component->numOutputs() ){
        SPDLOG_ERROR("channel out of bounds for component.");
        return false ;
    }

    sources_[device].erase({component, channel});
    return true ;
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

    std::string debug = "Current Topological Order: " ;
    for ( const auto c : topologicalOrder_ ){
        debug += fmt::format("{}, ", fmt::ptr(c));
    }
    SPDLOG_DEBUG(debug);
}

void SignalChain::reset(){
    sinks_.clear();
    visited_.clear();
    topologicalOrder_.clear();
}

void SignalChain::topologicalSort(
    AudioIO* module, 
    std::unordered_set<AudioIO*>& visited,
    std::vector<AudioIO*>& result    
){
    if ( visited.count(module) ) return ;
    visited.insert(module);

    // Process stateful modulators in signal chain (e.g., Oscillator)
    for ( AudioIO* m : module->getModulationInputs() ){
        topologicalSort(m, visited, result);
    }

    // Now process normal signal chain
    for ( size_t i = 0; i < module->numInputs(); ++i ){
        for ( const auto& conn : module->getInputs(i)){
            topologicalSort(conn.component, visited, result);
        }
    }
    
    result.push_back(module); // post-order traversal (only insert once all inputs are  visited)
}