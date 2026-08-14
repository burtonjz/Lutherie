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
    // true peripheral output sinks. key = peripheral output channel
    std::map<size_t, SignalConnectionSet> sinks_ ; 
    
    // topographical calculation sinks that don't contribute to a peripheral channel output
    // e.g., analysis, audio -> buffer
    SignalConnectionSet pseudoSinks_ ; 
    
    std::vector<AudioSignalComponent*> topologicalOrder_ ;
    std::unordered_set<AudioSignalComponent*> visited_  ;
    std::unordered_set<AudioSignalComponent*> modulatorOnly_ ;

public:
    SignalChain();

    void allocateOutputChannels(size_t numChannels);
    const std::vector<AudioSignalComponent*>& getSignalComponentChain() const ;
    const SignalConnectionSet& getSinks(size_t channel) const ;

    void addSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx);
    void removeSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx);

    void addPseudoSink(AudioSignalComponent* component, size_t channel);
    void removePseudoSink(AudioSignalComponent* component, size_t channel);

    void calculateTopologicalOrder();
    void reset();

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
    );
};

#endif // __SIGNAL_CHAIN_HPP_