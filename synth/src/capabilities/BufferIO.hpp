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
#include "capabilities/Capability.hpp"
#include "interfaces/BufferProcessor.hpp"
#include "interfaces/BufferObserver.hpp"
#include "interfaces/BufferRW.hpp"
#include "status/FileStatus.hpp"
#include "routing/BufferConnection.hpp"

class BufferIO : 
    public Capability,
    public BufferTrigger
{
private:
    enum class Event { Connected, Disconnected, Updated};

    BufferProcessor* processor_ ;
    std::vector<BufferObserver*> observers_ ;

    size_t numInputs_ ;
    size_t numOutputs_ ;

    std::vector<std::vector<double>> outputs_ ;
    std::vector<BufferStatus> status_ ;

    std::vector<std::vector<BufferRead>> inputViews_ ;
    std::vector<BufferRead>              outputViews_ ;

    // cache connections in class for edge walking
    std::vector<BufferConnectionSet> inputEdges_ ;
    std::vector<BufferConnectionSet> outputEdges_ ;

    double sampleRate_ ;

    BufferIO(BufferProcessor* processor, size_t inputs, size_t outputs);

public:
    // constructor for standard object. processor as reference to force valid
    BufferIO(BufferProcessor& processor, size_t inputs, size_t outputs);

    // constructor for special-case input-only BufferIO
    BufferIO(size_t inputs);

    void prepare(AudioContext audio);

    BufferIO(const BufferIO&) = delete ;
    BufferIO& operator=(const BufferIO&) = delete ;

    void addInputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel);
    void removeInputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel);

    void addOutputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel);
    void removeOutputEdge(BufferIO* edge, size_t edgeChannel, size_t toChannel);

    void addObserver(BufferObserver* obs);
    void removeObserver(BufferObserver* obs);
    void bindObserver(BufferObserver* obs);

    size_t numInputs() const ;
    size_t numOutputs() const ;

    FileStatus exportBuffer(std::string savePath);

    void trigger() override ;

private:
    friend class BufferAccessor ;
    friend class BufferObserver ;

    // start friend methods
    std::span<const BufferRead> inputView(size_t channel) const ;
    void rebuildInputViews(size_t channel);

    BufferRead outputView(size_t channel) const ;
    void rebuildOutputView(size_t channel);

    BufferResize outputResize(size_t channel);
    // end friend methods

    void process(Event e, size_t changedInput);
    void notifyObservers(Event e, size_t changedInput) const ;
    
    void publishOutputs();
    void notifyDownstream(); 
    

    size_t maxBufferFrames() const ;
    std::vector<double> interleavedBuffers(const SF_INFO& info) const ;

    void setStatus(size_t channel, BufferStatus s);
    void setAllStatus(BufferStatus s);
};

#endif // BUFFER_IO_HPP_