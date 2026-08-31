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

#include "api/StreamingApiHandler.hpp"
#include "config/Config.hpp"
#include "api/StreamingContext.hpp"
#include "core/ComponentManager.hpp"
#include "core/Engine.hpp"

#include <cstring>
#include <spdlog/spdlog.h>

StreamingApiHandler* StreamingApiHandler::instance(){
    static StreamingApiHandler* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new StreamingApiHandler();
    }
    return s_instance ;
}

StreamingApiHandler::StreamingApiHandler():
    udpSocket_(INVALID_SOCKET)
#ifdef _WIN32
    , wsaInitialized_(false)
#endif
{}


void StreamingApiHandler::start(){
    initSocket();

    SPDLOG_INFO(
        "Streaming API started, sending UDP to 127.0.0.1:{} (fd={})",
        ntohs(destAddr_.sin_port), udpSocket_
    );
    
    while (!Engine::stop_flag){
        StreamingApiHandler::instance()->processContexts();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    
    StreamingApiHandler::instance()->stop();
    SPDLOG_INFO("Stream thread stopping");

}

void StreamingApiHandler::stop(){
    closeSocket();
}

void StreamingApiHandler::initSocket() {
    Config::load();
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

void StreamingApiHandler::closeSocket() {
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

void StreamingApiHandler::processContexts(){
    auto ids = ComponentManager::instance()->getAudioProbeIds();

    for ( const auto& id : ids ){
        AudioProbe* a = ComponentManager::instance()->getAudioProbe(id);
        if ( !a ) continue ;
        StreamingContext* ctx = a->getStreamingContext();
        if ( !ctx ) continue ;

        size_t count = ctx->buffer.pop(ctx->scratch.data(), ctx->scratch.size());
        if ( count > 0 ){
            ctx->processFunc(ctx->scratch.data(), count, id);
        }
    }
}

void StreamingApiHandler::send(DataApiHeader header, const float* data, const size_t size){
    static const size_t headerSize = sizeof(DataApiHeader);
    static_assert(headerSize == 16, "DataApiHeader size changed -- check for padding before memcpy");

    if ( udpSocket_ == INVALID_SOCKET ){
        return ;
    }

    header.size = static_cast<uint64_t>(size * sizeof(float));

    // send header, then data as float
    std::vector<uint8_t> packet(headerSize + header.size);
    std::memcpy(packet.data(), &header, headerSize);
    if ( header.size > 0 ){
        std::memcpy(packet.data() + headerSize, data, header.size);
    }
    
    sendto(udpSocket_, packet.data(), packet.size(), 0,
        (struct sockaddr*)&destAddr_, sizeof(destAddr_));
}