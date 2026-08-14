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


#include <vector>
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

struct StreamingContext ;

class StreamingApiHandler {
private:
    static StreamingApiHandler* instance_ ;
    
    // UDP SOCKET VARIABLES
#ifdef _WIN32
    bool wsaInitialized_;
#endif
    SOCKET udpSocket_ ;
    struct sockaddr_in destAddr_ ;

    StreamingApiHandler();

public:
    static StreamingApiHandler* instance();

    StreamingApiHandler(const StreamingApiHandler&) = delete ;
    StreamingApiHandler& operator=(const StreamingApiHandler&) = delete ;
    StreamingApiHandler(StreamingApiHandler&&) = delete ;
    StreamingApiHandler& operator=(StreamingApiHandler&&) = delete ;

    void start();
    void stop();
    
    void processContexts();

    void send(const std::vector<float>& output, int componentId);
    
    void initSocket();
    void closeSocket();
 
};

#endif // ANALYTICS_ENGINE_HPP_