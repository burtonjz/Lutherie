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

#ifndef __SIGNAL_CHAIN_HPP_
#define __SIGNAL_CHAIN_HPP_

#include "core/AudioSignalComponent.hpp"

#include <unordered_set>
#include <vector>
#include <spdlog/spdlog.h>

// This class will store information regarding tracing a signal back to it's source
// (either a generator module or eventually an audio input), and handle order of  operations of ticking
// through modules

class SignalChain {
private:
    using OutboundNode = std::unordered_set<SignalConnection, ConnectionHash>;
    std::map<size_t, OutboundNode> outboundNodes_ ; // key=channel
    std::vector<AudioSignalComponent*> topologicalOrder_ ;
    std::unordered_set<AudioSignalComponent*> visited_  ;

    std::unordered_set<AudioSignalComponent*> modulatorOnly_ ;

public:
    SignalChain():
        outboundNodes_()
    {
    }

    void allocateOutputChannels(size_t numChannels){
        SPDLOG_DEBUG("allocating {} peripheral output channels", numChannels);
        for ( size_t i = 0 ; i < numChannels ; ++i ){
            if ( !outboundNodes_.contains(i) ){
                outboundNodes_[i];
            }
        }
    }

    std::vector<AudioSignalComponent*>& getSignalComponentChain(){
        return topologicalOrder_ ;
    }

    const std::unordered_set<SignalConnection, ConnectionHash>& getSinks(size_t channel) const {
        if ( !outboundNodes_.contains(channel) ){
            SPDLOG_ERROR("channel {} requested but outbound nodes only has {} allocated", channel, outboundNodes_.size());
        }
        assert(outboundNodes_.contains(channel));
        return outboundNodes_.at(channel);
    }

    void addSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx){
        if (!outbound){
            SPDLOG_WARN("Not adding a nullptr as a sink.");
            return ;
        }
        if ( outboundIdx > outbound->getNumOutputs() ){
            SPDLOG_WARN("outbound index out of bounds for module. Cannot add requested sink.");
            return ;
        }
        
        outboundNodes_[inboundIdx].insert({outbound, outboundIdx});
    }

    void removeSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx){
        if ( !outbound || outboundIdx > outbound->getNumOutputs() ){
            SPDLOG_WARN("outbound index out of bounds for specified module. Cannot remove sink.");
            return ; 
        }

        outboundNodes_[inboundIdx].erase({outbound, outboundIdx});
    }

    void calculateTopologicalOrder(){
        visited_.clear();
        topologicalOrder_.clear();
        
        // global post-order depth-first search
        for ( const auto& [channel, connections]: outboundNodes_ ){
            for ( const auto conn : connections ){
                topologicalSort(conn.component, visited_, topologicalOrder_);
            }
        }
    }

    void reset(){
        outboundNodes_.clear();
        visited_.clear();
        topologicalOrder_.clear();
    }

    

private:
    /**
     * @brief calculate the DAG via depth-first traversal from module to its inputs
     * 
     * @param module module pointer
     * @param visited tracks visits during traversal
     * @param result resulting ordered vector
     */
    void topologicalSort(
        AudioSignalComponent* module, 
        std::unordered_set<AudioSignalComponent*>& visited,
        std::vector<AudioSignalComponent*>& result    
    ){
        if (visited.count(module)) return ;
        visited.insert(module);

        // Process stateful modulators in signal chain (e.g., Oscillator)
        for (AudioSignalComponent* m : module->getModulationInputs() ){
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

};

#endif // __SIGNAL_CHAIN_HPP_