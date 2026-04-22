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


#ifndef ANALYTICS_ENGINE_HPP_
#define ANALYTICS_ENGINE_HPP_

#include "containers/LockFreeRingBuffer.hpp"
#include "types/ComponentType.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <kissfft/kiss_fft.h>

// Cross-platform socket includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int SOCKET ;
#endif

struct AnalysisContext {
    LockFreeRingBuffer<double> buffer ;
    std::function<void(const double*, size_t, int)> processFunc ;
    std::vector<double> scratch ;

    AnalysisContext(
        size_t bufferSize,
        size_t scratchSize,
        std::function<void(const double*, size_t, int)> func
    ) :
        buffer(bufferSize),
        processFunc(std::move(func)),
        scratch(scratchSize)
    {}
};

class AnalyticsEngine {
private:
    static AnalyticsEngine* instance_ ;

    std::mutex contextsMutex_ ;
    std::unordered_map<int, std::unique_ptr<AnalysisContext>> contexts_ ;
    
    // UDP SOCKET VARIABLES
#ifdef _WIN32
    bool wsaInitialized_;
#endif
    SOCKET udpSocket_ ;
    struct sockaddr_in destAddr_ ;

public:
    static AnalyticsEngine* instance();

    AnalyticsEngine(const AnalyticsEngine&) = delete ;
    AnalyticsEngine& operator=(const AnalyticsEngine&) = delete ;
    AnalyticsEngine(AnalyticsEngine&&) = delete ;
    AnalyticsEngine& operator=(AnalyticsEngine&&) = delete ;

    void start();
    void stop();

    void registerComponent(int componentId, ComponentType typ, std::function<void(const double*, size_t, int id)> func);
    void unregisterComponent(int componentId);
    
    void push(const double* data, size_t count, int componentId);
    void processContexts();

    void send(const std::vector<float>& output, int componentId);
    
private:
    AnalyticsEngine();
    ~AnalyticsEngine();
    
    void initSocket();
    void closeSocket();
 
};

#endif // ANALYTICS_ENGINE_HPP_