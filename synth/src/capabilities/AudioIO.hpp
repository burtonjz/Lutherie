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

#ifndef AUDIO_IO_HPP_
#define AUDIO_IO_HPP_

#include <cstddef>

#include "contexts/AudioContext.hpp"
#include "interfaces/AudioProcessor.hpp"
#include "interfaces/AudioObserver.hpp"
#include "interfaces/BufferRW.hpp"
#include "routing/AudioConnection.hpp"

class AudioIO {
private:
    AudioProcessor& processor_ ;
    std::vector<std::vector<AudioObserver*>> observers_ ;

    size_t currentFrame_ ;
    double sampleRate_ ;
    size_t bufferSize_ ;

    size_t numInputs_ ;
    size_t numOutputs_ ;

    std::vector<std::vector<double>> input_ ; // aggregated per-sample
    std::vector<std::vector<double>> output_ ;

    std::vector<BufferRead> inputView_ ;
    std::vector<BufferRead> outputView_ ;
    std::vector<BufferWrite> outputWrite_ ;

    AudioAccessor accessor_ ;

    std::vector<AudioConnectionSet> inputEdges_ ;
    std::vector<AudioConnectionSet> outputEdges_ ;

public:
    AudioIO(AudioProcessor& processor, AudioContext audio, size_t inputs, size_t outputs);
    
    AudioIO(const AudioIO&) = delete ;
    AudioIO& operator=(const AudioIO&) = delete ;

    void prepare(AudioContext audio);

    void addInputEdge(AudioIO* edge, size_t edgeChannel, size_t toChannel);
    void removeInputEdge(AudioIO* edge, size_t edgeChannel, size_t toChannel);

    void addOutputEdge(AudioIO* edge, size_t edgeChannel, size_t toChannel);
    void removeOutputEdge(AudioIO* edge, size_t edgeChannel, size_t toChannel);

    void addObserver(AudioObserver* observer, size_t channel);
    void removeObserver(AudioObserver* observer, size_t channel);

    size_t getNumInputs() const ;
    size_t getNumOutputs() const ;

    double getCurrentSample(size_t channel) const ;
    double getLastSample(size_t channel) const ;

    BufferRead output(size_t channel) const ;

    const AudioConnectionSet& getInputEdges(size_t channel) const ;
    const AudioConnectionSet& getOutputEdges(size_t channel) const ;

    void processFrame();
    void tick();
    
private:
    void aggregateInputFrame();
    void notifyObservers() const ;

    template <typename Span>
    static std::vector<Span> makeAccessor(std::vector<std::vector<double>>& src){
        std::vector<Span> views ;
        views.reserve(src.size());
        for (auto& v : src)
            views.emplace_back(v);
        return views ;
    }
};

#endif // AUDIO_IO_HPP_