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

#include <cstddef>
#include <cstring>
#include <memory>

#include "core/BaseComponent.hpp"
#include "config/Config.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json ;

// forward declaration
class Analyzer ;

struct SignalConnection {
    AudioSignalComponent* component ; // connecting module
    size_t index ; // buffer index (channel)

    bool operator==(const SignalConnection& other) const {
        return component == other.component && index == other.index ;
    }
};

struct ConnectionHash {
    std::size_t operator()(const SignalConnection& conn) const {
        return std::hash<AudioSignalComponent*>()(conn.component) ^ (std::hash<size_t>()(conn.index) << 1);
    }
};

using SignalConnectionSet = std::unordered_set<SignalConnection, ConnectionHash> ;

class AudioSignalComponent : public virtual BaseComponent {
protected:
    size_t bufferIndex_ ;
    size_t nInputs_ ;
    size_t nOutputs_ ;

    double sampleRate_ ;
    std::size_t bufferSize_ ;

    std::vector<SignalConnectionSet> signalInputs_ ;
    std::vector<SignalConnectionSet> signalOutputs_ ;
    std::vector<std::unique_ptr<double[]>> buffers_ ;

public:
    AudioSignalComponent(size_t in, size_t out);
    virtual ~AudioSignalComponent() = default ;

    void setBufferIndex(size_t index);
    
    virtual void calculateSample();

    double getCurrentSample(size_t output) const ;
    double getLastSample(size_t output) const ;

    virtual void clearBuffer();

    const double* data(size_t output = 0) const ;
    std::size_t size() const ;

    size_t getNumInputs() const ;
    size_t getNumOutputs() const ;

    void connectInput(AudioSignalComponent* source, size_t input, size_t sourceOutput);
    void disconnectInput(AudioSignalComponent* source, size_t input, size_t sourceOutput);

    const SignalConnectionSet& getInputs(size_t inp) const ;
    const SignalConnectionSet& getOutputs(size_t out) const ;

    virtual void tick();

    virtual bool isGenerative() const ;
    virtual bool isPolyphonic() const ;

    virtual void onInputConnect();
    virtual void onInputDisconnect();
    
protected:
    double aggregateInputs(size_t idx = 0) const ;
    void setBufferValue(size_t idx, double val);
};


inline void to_json(json& j, const SignalConnection& conn);

#endif // __MODULE_HPP_