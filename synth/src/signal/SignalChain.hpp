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

#include "core/AudioIO.hpp"
#include "types/ConnectionEndpoint.hpp"

#include <unordered_set>
#include <vector>
#include <optional>
#include <spdlog/spdlog.h>

// forward declarations
class AudioDevice ;

class SignalChain {
private:
    std::map<const ConnectionEndpoint, AudioConnectionSet, EndpointHash> sinks_ ; // components connected to device output
    std::map<const ConnectionEndpoint, AudioConnectionSet, EndpointHash> sources_ ; // components connected to device input
    AudioConnectionSet pseudoSinks_ ;  // topographical endpoints not connected to device (e.g., analysis, audio->buffer)) 
    
    std::vector<AudioIO*> topologicalOrder_ ;
    std::unordered_set<AudioIO*> visited_  ;
    std::unordered_set<AudioIO*> modulatorOnly_ ;

public:
    SignalChain();

    const std::vector<AudioIO*>& getSignalComponentChain() const ;

    const AudioConnectionSet& getSinks(const ConnectionEndpoint& device) const ;
    bool addSink(AudioIO* component, size_t channel, const ConnectionEndpoint& device);
    bool removeSink(AudioIO* component, size_t channel, const ConnectionEndpoint& device);

    bool addPseudoSink(AudioIO* component, size_t channel);
    bool removePseudoSink(AudioIO* component, size_t channel);

    const AudioConnectionSet& getSources(const ConnectionEndpoint& device) const ;
    bool addSource(AudioIO* component, size_t channel, const ConnectionEndpoint& device);
    bool removeSource(AudioIO* component, size_t channel, const ConnectionEndpoint& device);

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
        AudioIO* module, 
        std::unordered_set<AudioIO*>& visited,
        std::vector<AudioIO*>& result    
    );
};

#endif // __SIGNAL_CHAIN_HPP_