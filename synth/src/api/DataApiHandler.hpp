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

#ifndef DATA_API_HANDLER_HPP_
#define DATA_API_HANDLER_HPP_

#include "requests/DataDescriptor.hpp"

#include <unordered_set>
#include <vector>

class Engine ;

class DataApiHandler {
private:
    Engine* engine_ ;
    std::unordered_set<int> clientSockets_ ;

    DataApiHandler();

public:
    static DataApiHandler* instance();
    DataApiHandler(const DataApiHandler&) = delete ;
    DataApiHandler& operator=(const DataApiHandler&) = delete ;
    DataApiHandler(DataApiHandler&&) = delete ;
    DataApiHandler& operator=(DataApiHandler&&) = delete ;

    void initialize(Engine* engine);

    void start();
    void onClientConnection(int clientSock);
    void sendApiData(DataDescriptor header, const std::vector<double>& data);

private:
    bool sendAll(int sock, const uint8_t* data, size_t len);
};

#endif // DATA_API_HANDLER_HPP_
