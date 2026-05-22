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

#ifndef __MODULE_HPP_
#define __MODULE_HPP_

#include <cmath>
#include <cstddef>
#include <cstring>
#include <memory>
#include <algorithm>
#include <set>

#include "core/BaseComponent.hpp"
#include "config/Config.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json ;

// forward declaration
class Analyzer ;

struct SignalConnection {
    AudioSignalComponent* component ; // connecting module
    size_t index ; // buffer index 

    bool operator==(const SignalConnection& other) const {
        return component == other.component && index == other.index ;
    }
};
struct ConnectionHash {
    std::size_t operator()(const SignalConnection& conn) const {
        return std::hash<AudioSignalComponent*>()(conn.component) ^ (std::hash<size_t>()(conn.index) << 1);
    }
};

class AudioSignalComponent : public virtual BaseComponent {
protected:
    size_t bufferIndex_ ;
    size_t nInputs_ ;
    size_t nOutputs_ ;

    double sampleRate_ ;
    std::size_t bufferSize_ ;

    std::vector<std::unordered_set<SignalConnection, ConnectionHash>> signalInputs_ ;
    std::vector<std::unordered_set<SignalConnection, ConnectionHash>> signalOutputs_ ;
    std::vector<std::unique_ptr<double[]>> buffers_ ;

    std::set<std::pair<Analyzer*, size_t>> toAnalyzer_ ;

public:
    AudioSignalComponent(size_t in, size_t out):
        bufferIndex_(0),
        nInputs_(in),
        nOutputs_(out),
        signalInputs_(in),
        signalOutputs_(out),
        buffers_(out)
    {
        Config::load();
        sampleRate_ = Config::get<double>("audio.sample_rate").value();
        bufferSize_ = Config::get<size_t>("audio.buffer_size").value();

        for ( size_t i = 0; i < out; ++i){
            buffers_[i] = std::make_unique<double[]>(bufferSize_);
        }
    }
    
    virtual ~AudioSignalComponent() = default ;

    void setBufferIndex(size_t index){
        bufferIndex_ = index ;
    }
    
    virtual void calculateSample(){}

    double getCurrentSample(size_t output) const {
        assert( output < nOutputs_ );
        return buffers_[output][bufferIndex_];
    }

    double getLastSample(size_t output) const {
        assert( output < nOutputs_ );
        return buffers_[output][(bufferIndex_ + bufferSize_ - 1) % bufferSize_ ];
    }

    virtual void clearBuffer(){
        for ( auto& buf : buffers_ ){
            std::fill(buf.get(), buf.get() + bufferSize_, 0.0);
        }
    }

    const double* data(size_t output = 0) const {
        assert( output < nOutputs_ );
        return buffers_[output].get() ;
    }

    std::size_t size() const {
        return bufferSize_ ;
    }

    size_t getNumInputs() const {
        return nInputs_ ;
    }

    size_t getNumOutputs() const {
        return nOutputs_ ;
    }

    void connectInput(AudioSignalComponent* source, size_t input, size_t sourceOutput){
        assert( input < nInputs_ );
        assert( sourceOutput < source->nOutputs_ );
        signalInputs_[input].insert({source, sourceOutput});
        source->signalOutputs_[sourceOutput].insert({this, input});
    }

    void disconnectInput(AudioSignalComponent* source, size_t input, size_t sourceOutput){
        assert ( input < nInputs_ );
        assert ( sourceOutput < source->nOutputs_ );
        signalInputs_[input].erase({source, sourceOutput});
        source->signalOutputs_[sourceOutput].erase({this, input});
    }

    const std::set<std::pair<Analyzer*, size_t>>& getAnalyzers() const {
        return toAnalyzer_ ;
    }

    const std::unordered_set<SignalConnection, ConnectionHash>& getInputs(size_t inp) const {    
        assert( inp < nInputs_ );
        return signalInputs_[inp] ;
    }

    const std::unordered_set<SignalConnection, ConnectionHash>& getOutputs(size_t out) const {
        assert( out < nOutputs_ );
        return signalOutputs_[out] ;
    }

    virtual void tick(){
        bufferIndex_ = std::fmod(bufferIndex_ + 1, bufferSize_);
    }

    virtual bool isGenerative() const { return false; }
    virtual bool isPolyphonic() const { return false; }
    
protected:
    double aggregateInputs(size_t idx) const {
        assert( idx < nInputs_ );
        double sum = 0.0 ;
        for ( const auto& conn : signalInputs_[idx] ){
            sum += conn.component->getLastSample(conn.index);
        }
        return sum ;
    }

    void setBufferValue(size_t idx, double val){
        assert( idx < nOutputs_ );
        buffers_[idx][bufferIndex_] = val ;
    }

    void connectAnalyzer(Analyzer* a, size_t idx){
        toAnalyzer_.insert({a, idx});
    }

    void disconnectAnalyzer(Analyzer* a, size_t idx){
        toAnalyzer_.erase({a, idx});
    }
    friend class Analyzer ;
};


inline void to_json(json& j, const SignalConnection& conn){
    j["componentId"] = conn.component->getId() ;
    j["index"] = conn.index ;
};

#endif // __MODULE_HPP_