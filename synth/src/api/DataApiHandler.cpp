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

#include "api/DataApiHandler.hpp"
#include "config/Config.hpp"
#include "core/Engine.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <spdlog/spdlog.h>
#include <cstdio>

DataApiHandler::DataApiHandler()
{}

DataApiHandler* DataApiHandler::instance(){
    static DataApiHandler* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new DataApiHandler();
    }
    return s_instance ;
}

void DataApiHandler::initialize(Engine* engine){
    engine_ = engine ;
}

void DataApiHandler::start(){
    int serverPort = Config::get<int>("server.data_port").value();

    // Create socket
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == -1) {
        SPDLOG_WARN("Socket creation failed");
        exit(1);
    }
    int opt = 1 ;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        SPDLOG_WARN("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    // set socket as nonblocking
    fcntl(serverSock, F_SETFL, O_NONBLOCK);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);

    // Bind socket to address
    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        SPDLOG_WARN("Bind failed");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(serverSock, 3) < 0) {
        SPDLOG_WARN("Listen failed");
        return;
    }

    SPDLOG_INFO("Data API running on port {}... ", serverPort);

    // Accept incoming client connections in a loop
    while (!Engine::stop_flag) {
        int sock = accept(serverSock, nullptr, nullptr);
        if (sock >= 0) {
            // set client socket to non-blocking
            int flags = fcntl(sock, F_GETFL, 0);
            if ( flags == -1 ){
                perror("fcntl F_GETFL");
                close(sock);
                continue ;
            }
            if ( fcntl(sock, F_SETFL, flags | O_NONBLOCK ) == -1 ){
                perror("fcntl F_SETFL");
                close(sock);
                continue ;
            }

            // Handle client in a separate thread
            std::thread([sock](){
                DataApiHandler::instance()->onClientConnection(sock);
            }).detach();  
        } else {
            // no pending connection; sleep briefly to avoid busy-wait
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    // Close the server socket
    close(serverSock);
}

void DataApiHandler::onClientConnection(int sock){
    char buffer[1024];
    clientSockets_.insert(sock);

    while (!Engine::stop_flag){
        ssize_t bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0) ;
        if (bytesReceived > 0 ){
            // no inbound messages expected: drain/discard
            SPDLOG_WARN("Data API client {} sent {} unexpected bytes", sock, bytesReceived);
        } else if ( bytesReceived == 0){
            break ; // peer closed
        } else {
            if ( errno == EAGAIN || errno == EWOULDBLOCK ){
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue ;
            } else {
                perror("recv");
                break ;
            }
        }
    }

    clientSockets_.erase(sock);
    close(sock);
}

void DataApiHandler::sendApiData(DataDescriptor header, const std::vector<double>& data){
    static_assert(sizeof(DataDescriptor) == 16, "DataDescriptor size changed -- check for padding before memcpy");

    header.size = static_cast<uint64_t>(data.size() * sizeof(double));

    std::vector<uint8_t> buffer(sizeof(DataDescriptor) + header.size);
    std::memcpy(buffer.data(), &header, sizeof(DataDescriptor));
    if ( header.size > 0 ){
        std::memcpy(buffer.data() + sizeof(DataDescriptor), data.data(), header.size);
    }

    for ( const auto& sock : clientSockets_ ){
        if ( !sendAll(sock, buffer.data(), buffer.size()) ){
            SPDLOG_WARN("failure sending buffer data of size {} to socket {}", 
                buffer.size(), 
                sock
            );
        }
    }

    SPDLOG_DEBUG("Data API sent buffer of size {} for component {} channel {}", 
        buffer.size(), 
        header.componentId, 
        header.channel
    );
}

bool DataApiHandler::sendAll(int sock, const uint8_t* data, size_t len){
    size_t totalSent = 0 ;
    while ( totalSent < len ){
        ssize_t sent = send(sock, data + totalSent, len - totalSent, 0);
        if ( sent > 0 ){
            totalSent += static_cast<size_t>(sent);
        } else if ( sent < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK )){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else if ( errno == EINTR ){
            continue ;
        } else {
            perror("send");
            return false ;
        }
    }
    return true ;
}