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

#include "core/AudioDevice.hpp"
#include "containers/LockFreeRingBuffer.hpp"
#include "signal/SignalController.hpp"
#include "config/Config.hpp"

#include <spdlog/spdlog.h>
#include <samplerate.h>

AudioDevice::AudioDevice(const RtAudio::DeviceInfo& info):
    type_(),
    bufferSize_(Config::get<uint32_t>("audio.buffer_size").value()),
    globalRate_(Config::get<uint32_t>("audio.sample_rate").value()),
    itf_(),
    info_(info),
    inputParams_(std::nullopt),
    outputParams_(std::nullopt),
    options_()
{
    setType(info);
}

RtAudio& AudioDevice::interface(){
    return itf_ ;
}

const RtAudio::DeviceInfo& AudioDevice::info() const {
    return info_ ;
}

AudioDevice::Type AudioDevice::type() const {
    return type_ ;
}

void AudioDevice::setType(AudioDevice::Type typ){
    type_ = typ ;

    if ( type() != AudioDevice::DAC ){
        setDefaultInputParams();
    } else {
        inputParams_ = std::nullopt ;
    }

    if ( type() != AudioDevice::ADC ){
        setDefaultOutputParams();
    } else {
        inputParams_ = std::nullopt ;
    }

    configureEndpoints();
}

void AudioDevice::setType(const RtAudio::DeviceInfo& info){
    if      ( info.duplexChannels > 0 ) setType(AudioDevice::ADAC);
    else if ( info.outputChannels > 0 ) setType(AudioDevice::ADC);
    else                                setType(AudioDevice::DAC);
}

bool AudioDevice::isOutputDevice() const {
    return type_ == DAC || type_ == ADAC ;
}

bool AudioDevice::isInputDevice() const {
    return type_ == ADC || type_ == ADAC ;
}

uint32_t AudioDevice::inboundChannels() const {
    if ( inputParams_.has_value() ) return inputParams_->nChannels ;
    return 0 ;
}

uint32_t AudioDevice::outboundChannels() const {
    if ( outputParams_.has_value() ) return outputParams_->nChannels ;
    return 0 ;
}

size_t AudioDevice::bufferSize() const {
    return bufferSize_ ;
}

size_t AudioDevice::sampleRate() const {
    return sampleRate_ ;
}

bool AudioDevice::isGlobalSampleRate() const {
    return matchesGlobalRate_ ;
}

void AudioDevice::setDefaultInputParams(){
    if ( !inputParams_ ){
        inputParams_ = RtAudio::StreamParameters();
    }
    
    inputParams_->deviceId = info().ID ;
    
    // virtual devices may have absurd numbers of channels
    // but can show up in RtAudio. Force to reasonable in this case
    inputParams_->nChannels = info().inputChannels ;
    inputParams_->firstChannel = 0 ;
    if ( inputParams_->nChannels > 8 ){
        SPDLOG_WARN(
            "selected device '{}' reported {} input channels. "
            "this is outside of a typical range, and suggests "
            "this may be a virtual device. Setting to one channel.",
            info().name, inputParams_->nChannels 
        );    
        inputParams_->nChannels = 1 ;
    }
}

void AudioDevice::setDefaultOutputParams(){
    if ( !outputParams_ ){
        outputParams_ = RtAudio::StreamParameters();
    }
    
    outputParams_->deviceId = info().ID ;
    
    // virtual devices may have absurd numbers of channels
    // but can show up in RtAudio. Force to reasonable in this case
    outputParams_->nChannels = info().outputChannels ;
    outputParams_->firstChannel = 0 ;
    if ( outputParams_->nChannels > 8 ){
        SPDLOG_WARN(
            "selected device '{}' reported {} output channels. "
            "this is outside of a typical range, and suggests "
            "this may be a virtual device. Setting to one channel.",
            info().name, outputParams_->nChannels 
        );    
        outputParams_->nChannels = 2 ;
    } 
}

void AudioDevice::configureEndpoints(){
    inbound_.clear();
    for ( uint32_t i = 0 ; i < inboundChannels(); ++i ){
        inbound_.push_back(ConnectionEndpoint::create(
            EndpointType::DEVICE, 
            SocketType::SignalInbound,
            info_.ID
        ));
    }

    outbound_.clear();
    for ( uint32_t i = 0 ; i < outboundChannels(); ++i ){
        outbound_.push_back(ConnectionEndpoint::create(
            EndpointType::DEVICE, 
            SocketType::SignalOutbound,
            info_.ID
        ));
    }
}

bool AudioDevice::hasConnection() const {
    return connectionCount > 0 ;
}

void AudioDevice::addConnection(){
    ++connectionCount ;
}

void AudioDevice::removeConnection(){
    if ( connectionCount > 0 ) --connectionCount ;
}

const std::vector<ConnectionEndpoint>& AudioDevice::inboundEndpoints() const {
    return inbound_ ;
}

const std::vector<ConnectionEndpoint>& AudioDevice::outboundEndpoints() const {
    return outbound_ ;
}

LockFreeRingBuffer<double>& AudioDevice::outboundBuffer(size_t channel){
    return outboundRings_.at(channel);
}

std::vector<double>& AudioDevice::outboundScratch(size_t channel){
    return outboundScratch_.at(channel);
}

void AudioDevice::init(){
    // validate sample rate
    globalRate_ = Config::get<uint32_t>("audio.sample_rate").value();
    auto& rates = info_.sampleRates ;

    matchesGlobalRate_ = std::count(rates.begin(), rates.end(), globalRate_) > 0 ;
    if ( !matchesGlobalRate_ ){
        sampleRate_ = info_.preferredSampleRate ;
        SPDLOG_WARN(
            "Global sample rate of {} is not supported by device {}." 
            "Setting to device preferred sample rate of {}, and this device "
            "will require resampling.",
            globalRate_, info_.name, sampleRate_ 
        );

        initResample();
    } else {
        sampleRate_ = globalRate_ ;
    }

    options_.flags = RTAUDIO_SCHEDULE_REALTIME ;
    options_.priority = 50 ;
}

void AudioDevice::initResample(){
    resampleRatio_ = static_cast<double>(globalRate_) / sampleRate_ ;
    resampleScratchInp_.resize(bufferSize_);
    resampleScratchOut_.resize(bufferSize_ * resampleRatio_ + 1);

    const size_t nChannels = inboundChannels();
    int err = 0 ;
    resampleSrc_ = src_new(SRC_SINC_MEDIUM_QUALITY, int(nChannels), &err);
    if ( !resampleSrc_ ){
        SPDLOG_ERROR("src_new failed: {}", src_strerror(err));
        return ;
    }

    resampleRatio_ = double(globalRate_) / sampleRate_ ;
}

bool AudioDevice::start(RtAudioCallback callback){
    void* userData = static_cast<void*>(this);


    if ( itf_.openStream(
        outputParams_ ? &*outputParams_ : nullptr,
        inputParams_ ? &*inputParams_ : nullptr,
        RTAUDIO_FLOAT64, sampleRate_,
        &bufferSize_,
        callback,
        userData,
        &options_
    )){
        SPDLOG_ERROR(
            "Error opening stream for device '{}': {}",
            info().name, itf_.getErrorText()
        );
        running_ = false ;
        return running_ ;
    }

    // allocate buffers based on response from stream open
    const size_t nChannels = outboundChannels();
    const size_t ringBufferSize = Config::get<size_t>("audio.input_ring_size").value();
    const size_t maxOut = size_t(std::ceil(
        bufferSize_ * resampleRatio_ * maxResampleRatioDeviation)) 
        + numResampleSlackFrames ;

    outboundRings_.clear();
    for ( size_t i = 0; i < nChannels; ++i ){
        outboundRings_.emplace_back(ringBufferSize);
    }

    outboundScratch_.assign(
        nChannels, 
        std::vector<double>(bufferSize_)
    );

    resampleScratchInp_.assign(
        bufferSize_ * nChannels, 
        0.f
    );

    resampleScratchOut_.assign(
        maxOut * nChannels, 
        0.f
    );
    
    if ( itf_.startStream() ){
        SPDLOG_ERROR(
            "Error starting stream for device '{}': {}",
            info().name, itf_.getErrorText()
        );
        if ( itf_.isStreamOpen() ) itf_.closeStream();
        running_ = false ;
        return running_ ;
    }

    return 0 ;
}

void AudioDevice::stop(){
    if ( itf_.isStreamRunning() ) itf_.stopStream();
    if ( itf_.isStreamOpen() ) itf_.closeStream();
}

bool AudioDevice::running() const {
    return running_ ;
}

size_t AudioDevice::resample(const double* in, size_t frames, size_t nChannels){
    if ( frames > bufferSize_ ) return 0 ;

    std::transform(
        in, in + frames * nChannels, 
        resampleScratchInp_.begin(), [](double x){
            return static_cast<float>(x);
    });

    SRC_DATA data{};
    data.data_in       = resampleScratchInp_.data();
    data.input_frames  = long(frames);
    data.data_out      = resampleScratchOut_.data();
    data.output_frames = long(resampleScratchOut_.size() / nChannels);
    data.src_ratio     = resampleRatio_ ;
    data.end_of_input  = 0 ;

    if ( 
        src_process(resampleSrc_, &data) != 0 || 
        data.input_frames_used != long(frames) 
    ) return 0 ;

    return size_t(data.output_frames_gen);
}

void AudioDevice::pushToRingBuffer(const auto* data, size_t frames){
    for ( auto& buf : outboundRings_ ){
        if ( buf.writeSpace() < frames ){
            return ;
        }
    }

    for ( size_t i = 0; i < outboundRings_.size(); ++i ){
        outboundRings_[i].pushStrided(data + i, outboundChannels(), frames);
    }
}

const std::vector<double>& AudioDevice::processFrame(){
    // Step 1: source nodes ingest from ring buffers
    SignalController::instance()

    // Step 2: Signal Controller 
    SignalController::instance()->processSignalChain();
    // 
    
    for ( const auto& ep : outputDevice->inboundEndpoints() ){
        auto& output = outputs_[ep];
        output = 0.0 ;
        for ( const auto& conn : signalChain_.getSinks(ep) ){
            output += conn.component->getLastSample(conn.index);
        }
    }
    return {outputs_.data(), outputs_.size()} ;
}