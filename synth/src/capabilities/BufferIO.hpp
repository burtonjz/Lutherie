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

#include "contexts/AudioContext.hpp"
#include "interfaces/BufferProcessor.hpp"
#include "interfaces/BufferObserver.hpp"
#include "interfaces/BufferRW.hpp"
#include "status/FileStatus.hpp"
#include "routing/BufferConnection.hpp"

class BufferIO {
private:
    enum class Hook { Connected, Disconnected, Updated };

    BufferProcessor& processor_ ;
    std::vector<BufferObserver*> observers_ ;

    size_t numInputs_ ;
    size_t numOutputs_ ;

    std::vector<std::vector<double>> outputs_ ;
    std::vector<BufferStatus> status_ ;

    // cache connections in class for edge walking
    std::vector<BufferConnectionSet> inputEdges_ ;
    std::vector<BufferConnectionSet> outputEdges_ ;

    double sampleRate_ ;

public:
    BufferIO(AudioContext audio, BufferProcessor& processor, size_t inputs, size_t outputs);

    void addInputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel);
    void removeInputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel);

    void addOutputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel);
    void removeOutputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel);

    void addObserver(BufferObserver* observer);
    void removeObserver(BufferObserver* observer);

    size_t getNumInputs() const ;
    size_t getNumOutputs() const ;

    FileStatus exportBuffer(std::string savePath);
        
private:
    friend class BufferContext ;

    BufferReads createInputsReadOnly(size_t channel) const ;

    std::optional<BufferRead> createOutputReadOnly(size_t channel) const ;
    BufferReads createOutputsReadOnly() const ;

    BufferWrite createOutputWrite(size_t channel);
    BufferWrites createOutputsWrite();

    void process(Hook h, size_t changedInput);
    void notifyDownstream();

    size_t getMaxBufferFrames() const ;
    std::vector<double> getInterleavedBuffers(const SF_INFO& info) const ;

    void notifyObservers();
};

#endif // BUFFER_IO_HPP_