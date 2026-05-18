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

#include "dsp/Analyzer.hpp"
#include "dsp/AnalyticsEngine.hpp"
#include "core/AudioStreamComponent.hpp"

 Analyzer::Analyzer(ComponentId id, ComponentType typ):
    BaseComponent(id, typ),
    buffer_(nullptr),
    sources_()
{
    Config::load();
    sampleRate_ = Config::get<double>("audio.sample_rate").value();
    bufferSize_ = Config::get<size_t>("audio.buffer_size").value();

    buffer_ = std::make_unique<double[]>(bufferSize_);

    AnalyticsEngine::instance()->registerComponent(
        getId(),
        getType(),
        [this](const double* data, size_t size, ComponentId id){
            process(data, size, id);
        }
    );
}

Analyzer::~Analyzer(){
    AnalyticsEngine::instance()->unregisterComponent(getId());
};

void Analyzer::aggregateInputs(){
    for ( auto [s, index] : sources_ ){
        auto sBuf = s->data(index);
        std::transform(
            sBuf, sBuf + bufferSize_,
            buffer_.get(), buffer_.get(), std::plus<double>{}
        );                
    }
}

void Analyzer::clearBuffer(){
    std::fill(buffer_.get(), buffer_.get() + bufferSize_, 0.0);
}

const double* Analyzer::data() const {
    return buffer_.get() ;
}

std::size_t Analyzer::size() const {
    return bufferSize_ ;
}

void Analyzer::connectInput(AudioStreamComponent* source, size_t index){
    if ( !source ) return ;

    auto s = std::make_pair(source, index);

    auto it = std::find(sources_.begin(), sources_.end(), s);
    if ( it != sources_.end() ) return ; 

    sources_.push_back(s);
    source->connectAnalyzer(this, index);
}

void Analyzer::disconnectInput(AudioStreamComponent* source, size_t index){
    if ( !source ) return ;

    auto s = std::make_pair(source, index);

    auto it = std::find(sources_.begin(), sources_.end(), s);
    if ( it == sources_.end() ) return ;

    sources_.erase(it);
    source->disconnectAnalyzer(this, index);
}

void Analyzer::flush(){
    clearBuffer();
    aggregateInputs();
    AnalyticsEngine::instance()->push(data(), size(), getId());
}

const std::vector<std::pair<AudioStreamComponent*, size_t>>& Analyzer::getSources() const {    
    return sources_ ;
}