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

#include "dsp/AnalyticsEngine.hpp"
#include "config/Config.hpp"
#include <cstring>
#include <spdlog/spdlog.h>

AnalyticsEngine* AnalyticsEngine::instance(){
    static AnalyticsEngine* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new AnalyticsEngine();
    }
    return s_instance ;
}

AnalyticsEngine::AnalyticsEngine():
    udpSocket_(INVALID_SOCKET)
#ifdef _WIN32
    , wsaInitialized_(false)
#endif
{
}

AnalyticsEngine::~AnalyticsEngine(){
    stop();
}

void AnalyticsEngine::start(){
    initSocket();
    SPDLOG_INFO("AnalyticsEngine started on UDP port {}", udpSocket_);
}

void AnalyticsEngine::stop(){
    closeSocket();
}

void AnalyticsEngine::registerComponent(int componentId, ComponentType typ, std::function<void(const double*, size_t, int id)> func){
    std::lock_guard<std::mutex> lock(contextsMutex_);
    if ( contexts_.contains(componentId) ) return ;
    
    auto size = Config::get<unsigned int>("analysis.ring_buffer_size").value_or(480000);
    size_t scratchSize ;
    switch(typ){
    case ComponentType::SpectrumAnalyzer:
        scratchSize = Config::get<int>("analysis.spectrum_analyzer.buffer_size").value();
        break ;
    case ComponentType::Oscilloscope:
        scratchSize = Config::get<int>("analysis.oscilloscope.buffer_size").value();
        break ;
    default:
        SPDLOG_WARN("unknown component type {} registered with analytics engine. Unknown buffer size");
        scratchSize = 0 ;
        break ;
    }
    contexts_.emplace(componentId, std::make_unique<AnalysisContext>(size,scratchSize,std::move(func)));
}

void AnalyticsEngine::unregisterComponent(int componentId){
    pendingRemove_.insert(componentId);
}

void AnalyticsEngine::initSocket() {
#ifdef _WIN32
    if ( !wsaInitialized_ ) {
        WSADATA wsaData;
        if ( WSAStartup(MAKEWORD(2, 2), &wsaData ) != 0 ) {
            SPDLOG_ERROR("WSAStartup failed");
            return ;
        }
        wsaInitialized_ = true ;
    }
#endif
    
    udpSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if ( udpSocket_ == INVALID_SOCKET ) {
        SPDLOG_ERROR("UDP socket creation failed");
        return ;
    }
    
    // Set up destination address (localhost)
    memset(&destAddr_, 0, sizeof(destAddr_));
    destAddr_.sin_family = AF_INET ;
    destAddr_.sin_port = htons(Config::get<unsigned int>("analysis.port").value_or(54322));
    
#ifdef _WIN32
    destAddr_.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
    inet_pton(AF_INET, "127.0.0.1", &destAddr_.sin_addr);
#endif
}

void AnalyticsEngine::closeSocket() {
    if (udpSocket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(udpSocket_);
#else
        close(udpSocket_);
#endif
        udpSocket_ = INVALID_SOCKET ;
    }
    
#ifdef _WIN32
    if ( wsaInitialized_ ) {
        WSACleanup();
        wsaInitialized_ = false ;
    }
#endif
}

void AnalyticsEngine::push(const double* data, size_t count, int componentId){
    std::lock_guard<std::mutex> lock(contextsMutex_);
    auto it = contexts_.find(componentId);
    if ( it == contexts_.end() ) return ;

    it->second->buffer.push(data, count);
}

void AnalyticsEngine::processContexts(){
    std::lock_guard<std::mutex> lock(contextsMutex_);
    for ( auto& [id, ctx] : contexts_ ){
        size_t count = ctx->buffer.pop(ctx->scratch.data(), ctx->scratch.size());
        if ( count > 0 ){
            ctx->processFunc(ctx->scratch.data(), count, id);
        }
    } 

    // resolve pending unregisterations
    for ( int id : pendingRemove_ ){
        contexts_.erase(id);
    }
    pendingRemove_.clear();
}

void AnalyticsEngine::send(const std::vector<float>& output, int componentId) {
    if ( udpSocket_ == INVALID_SOCKET ){
        return ;
    }

    // ComponentId as int32, then data as float: [float, float, float, ...]
    size_t dataSize = sizeof(int32_t) + output.size() * sizeof(float);
    std::vector<char> packet(dataSize);

    int32_t id = static_cast<int32_t>(componentId);
    std::memcpy(packet.data(), &id, sizeof(int32_t));
    std::memcpy(packet.data() + sizeof(int32_t), output.data(), output.size() * sizeof(float));
    
    sendto(udpSocket_, packet.data(), static_cast<int>(dataSize), 0,
        (struct sockaddr*)&destAddr_, sizeof(destAddr_));
}