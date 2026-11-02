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

#ifndef BUFFER_IO_HPP_
#define BUFFER_IO_HPP_

#include <sndfile.h>
#include <spdlog/spdlog.h>
#include <vector>

#include "routing/BufferConnection.hpp"

class BufferIO {
protected:
    size_t nInputs_ ;
    size_t nOutputs_ ;
    
    std::vector<BufferConnectionSet> inboundConnections_ ;
    std::vector<BufferConnectionSet> outboundConnections_ ;
    std::vector<std::vector<double>> buffers_ ;

    double sampleRate_ ;

public:
    BufferIO(size_t in, size_t out);

    bool connectInput(BufferIO* source, size_t input, size_t sourceOutput);
    bool disconnectInput(BufferIO* source, size_t input, size_t sourceOutput);
    
    const std::vector<double>& getBuffer(size_t idx) const ;

    size_t getNumInputs() const ;
    size_t getNumOutputs() const ;
    
    const BufferConnectionSet& getInputs(size_t inp) const ;
    const BufferConnectionSet& getOutputs(size_t out) const ;

    bool saveBuffer(std::string savePath);
        
protected:
    virtual void onInputConnect();
    virtual void onInputDisconnect();
    virtual void onInputUpdated();

    void notifyDownstream(size_t index);

    size_t getMaxBufferFrames() const ;
    std::vector<double> getInterleavedBuffers(const SF_INFO& info) const ;
        
};

#endif // BUFFER_IO_HPP_