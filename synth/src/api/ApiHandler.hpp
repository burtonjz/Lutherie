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

#ifndef __API_HANDLER_HPP_
#define __API_HANDLER_HPP_

#include <nlohmann/json.hpp>
#include <functional>

#include "core/BaseComponent.hpp"
#include "meta/CollectionDescriptor.hpp"
#include "requests/ConnectionRequest.hpp"
#include "requests/CollectionRequest.hpp"
#include "params/ParameterMap.hpp"


using json = nlohmann::json ;

// forward declarations
class Engine ;

class ApiHandler {
private:
    using HandlerFunc = std::function<json(int sock, const json& request)>;
    Engine* engine_ ;
    std::unordered_map<std::string, HandlerFunc> handlers_ ;

    ApiHandler();

public:
    static ApiHandler* instance() ;
    ApiHandler(const ApiHandler&) = delete ;
    ApiHandler& operator=(const ApiHandler&) = delete ;
    ApiHandler(ApiHandler&&) = delete ;
    ApiHandler& operator=(ApiHandler&&) = delete ;

    void initialize(Engine* engine);

    void start();
    void onClientConnection(int clientSock);
    void handleClientMessage(int clientSock, std::string jsonStr);
    json sendApiResponse(int clientSock, json& response, const std::string& err = "");

private:
    /*
    ---------------------------------------------------------
    --------------------HANDLER FUNCTIONS--------------------
    ---------------------------------------------------------
    */
    // engine management
    json getAudioDevices(int sock, const json& request);
    json getMidiDevices(int sock, const json& request);
    json setAudioDevice(int sock, const json& request);
    json setMidiDevice(int sock, const json& request);
    json setState(int sock, const json& request);
    // api save/load
    json getConfiguration(int sock, const json& request);
    json loadPatch(int sock, const json& request);
    // component management
    json addComponent(int sock, const json& request);
    json removeComponent(int sock, const json& request);
    json parseConnectionRequest(int sock, const json& request);
    bool routeConnectionRequest(ConnectionRequest request);
    json syncComponent(int sock, const json& request);
    
    // parameter management
    json getParameter(int sock, const json& request);
    json setParameter(int sock, const json& request);
    json getParameterDefault(int sock, const json& request);
    json setParameterDefault(int sock, const json& request);
    json getParameterValueRange(int sock, const json& request);
    json setParameterValueRange(int sock, const json& request);
    json resetParameter(int sock, const json& request);    
    // collection management
    json parseCollectionRequest(int sock, const json& request);
    json addCollectionValue(int sock, BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request);
    json removeCollectionValue(int sock, BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request);
    json getCollectionValue(int sock, BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request);
    json setCollectionValue(int sock, BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request);
    json resetCollection(int sock, BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request);
    json getCollectionValueRange(int sock, BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request);
    // modulation management
    json getModulationStrategy(int sock, const json& request);
    json setModulationStrategy(int sock, const json& request);
    json getModulationDepth(int sock, const json& request);
    json setModulationDepth(int sock, const json& request);
    
    // load functions
    bool loadCreateComponent(int sock, const json& components, std::unordered_map<int,int>& idMap);
    bool loadConnectComponent(int sock, const json& config);
    void loadUpdateIds(json& j, const std::unordered_map<int, int>& idMap);

    // collection helpers
    const CollectionDescriptor& getCollectionDescriptor(ComponentType t, CollectionType c) const ;
    bool validateCollectionJson(const json& j, CollectionStructure s) const ;
};


#endif // __API_HANDLER_HPP_
