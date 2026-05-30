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

#ifndef AUDIO_BUFFER_COMPONENT_HPP_
#define AUDIO_BUFFER_COMPONENT_HPP_

#include "core/BaseComponent.hpp"

#include <spdlog/spdlog.h>
#include <vector>

class AudioBufferComponent ;

struct BufferConnection {
    AudioBufferComponent* component ;
    size_t index ;

    bool operator==(const BufferConnection& other) const {
        return component == other.component && index == other.index ;
    }
};

struct BufferHash {
    std::size_t operator()(const BufferConnection& conn) const {
        return std::hash<AudioBufferComponent*>()(conn.component) ^ (std::hash<size_t>()(conn.index) << 1);
    }
};

class AudioBufferComponent: public virtual BaseComponent {
protected:
    size_t nInputs_ ;
    size_t nOutputs_ ;
    
    std::vector<std::unordered_set<BufferConnection, BufferHash>> inboundConnections_ ;
    std::vector<std::unordered_set<BufferConnection, BufferHash>> outboundConnections_ ;
    std::vector<std::vector<double>> buffers_ ;

    double sampleRate_ ;

public:
    AudioBufferComponent(size_t in, size_t out):
        nInputs_(in),
        nOutputs_(out),
        inboundConnections_(in),
        outboundConnections_(out),
        buffers_(out)
    {
        Config::load();
        sampleRate_ = Config::get<double>("audio.sample_rate").value();
    }

    void connectInput(AudioBufferComponent* source, size_t input, size_t sourceOutput){
        assert( input < nInputs_ );
        assert( sourceOutput < source->nOutputs_ );
        inboundConnections_[input].insert({source, sourceOutput});
        source->outboundConnections_[sourceOutput].insert({this, input});
    }

    void disconnectInput(AudioBufferComponent* source, size_t input, size_t sourceOutput){
        assert ( input < nInputs_ );
        assert ( sourceOutput < source->nOutputs_ );
        inboundConnections_[input].erase({source, sourceOutput});
        source->outboundConnections_[sourceOutput].erase({this, input});
    }

    const std::vector<double>& getBuffer(size_t idx) const {
        if ( idx >= nOutputs_ ){
            std::string err = fmt::format("Cannot get buffer of index {}. Out of range (size={})", idx, nOutputs_);
            SPDLOG_CRITICAL(err);
            throw std::runtime_error(err);
        }

        return buffers_.at(idx);
    }

    size_t getNumInputs() const {
        return nInputs_ ; 
    }

    size_t getNumOutputs() const {
        return nOutputs_ ;
    }
    
    const std::unordered_set<BufferConnection, BufferHash>& getInputs(size_t inp) const {    
        assert( inp < nInputs_ );
        return inboundConnections_[inp] ;
    }

    const std::unordered_set<BufferConnection, BufferHash>& getOutputs(size_t out) const {
        assert( out < nOutputs_ );
        return outboundConnections_[out] ;
    }
};

#endif // AUDIO_BUFFER_COMPONENT_HPP_