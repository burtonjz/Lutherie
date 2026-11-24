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

#ifndef CONNECTION_MANAGER_HPP_
#define CONNECTION_MANAGER_HPP_

// forward declarations
class ConnectionRequest ;

// THINGS TO CAPTURE IN HERE
// modulation:
// std::set<ModulationTarget>& getModulationTargets(){
//         return modulated_ ;
//     }
// 
class ConnectionManager {
private:

public:
    // bool handleConnection(const ConnectionRequest& connection);
    // byId(int id, DeviceType typ);
    // bool handleMidiConnection(ConnectionRequest connection);
    // std::vector<ConnectionRequest> getComponentConnections(int id) const ;
    // void getComponentConnections(int id, std::vector<ConnectionRequest>& requests) const ;
    // bool handleModulationConnection(ConnectionRequest request);
    // bool handleBufferConnection(ConnectionRequest request);
};

#endif // CONNECTION_MANAGER_HPP_