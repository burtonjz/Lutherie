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

#ifndef CONTROL_API_HANDLER_HPP_
#define CONTROL_API_HANDLER_HPP_

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

class ControlApiHandler {
private:
    using HandlerFunc = std::function<json(const json& request)>;
    Engine* engine_ ;
    std::unordered_map<std::string, HandlerFunc> handlers_ ;
    std::unordered_set<int> clientSockets_ ;

    ControlApiHandler();

public:
    static ControlApiHandler* instance();
    ControlApiHandler(const ControlApiHandler&) = delete ;
    ControlApiHandler& operator=(const ControlApiHandler&) = delete ;
    ControlApiHandler(ControlApiHandler&&) = delete ;
    ControlApiHandler& operator=(ControlApiHandler&&) = delete ;

    void initialize(Engine* engine);

    void start();

    const std::unordered_set<int>& getOpenClientSockets() const ;
    void handleClientMessage(const std::string& jsonStr);

    /*
    ---------------------------------------------------------
    --------------------HANDLER FUNCTIONS--------------------
    ---------------------------------------------------------
    */
    // engine management
    json getAudioDevices(const json& request);
    json setAudioDevice(const json& request);
    json getAudioConfig(const json& request);
    json getMidiDevices(const json& request);
    json setMidiDevice(const json& request);
    json setState(const json& request);

    // api save/load
    json getConfiguration(const json& request);
    json loadPatch(const json& request);

    // component management
    json addComponent(const json& request);
    json removeComponent(const json& request);
    json parseConnectionRequest(const json& request);
    bool routeConnectionRequest(ConnectionRequest request);
    json syncComponent(const json& request);
    
    // parameter management
    json getParameter(const json& request);
    json setParameter(const json& request);
    json getParameterDefault(const json& request);
    json setParameterDefault(const json& request);
    json getParameterValueRange(const json& request);
    json setParameterValueRange(const json& request);
    json resetParameter(const json& request);    

    // collection management
    json parseCollectionRequest(const json& request);
    json addCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request);
    json removeCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request);
    json getCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request);
    json getCollectionValues(BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request);
    json setCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request);
    json addCollectionValues(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request);
    json resetCollection(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request);

    // modulation management
    json getModulationStrategy(const json& request);
    json setModulationStrategy(const json& request);
    json getModulationDepth(const json& request);
    json setModulationDepth(const json& request);
    
    // file management
    json getFilePath(const json& request);
    json setFilePath(const json& request);

    // data api
    json getBufferData(const json& request);

    // midi control messages
    json midiLearn(const json& request);
    json getMidiControl(const json& request);
    json setMidiControl(const json& request);
    
private:
    void onClientConnection(int clientSock);
    json sendApiResponse(json& response, const std::string& err = "");
    
    // load functions
    bool loadCreateComponent(const json& components, std::unordered_map<int,int>& idMap);
    bool loadConnectComponent(const json& config);
    bool loadMidiControls(const json& controls);
    void loadUpdateIds(json& j, const std::unordered_map<int, int>& idMap);

    // collection helpers
    const CollectionDescriptor& getCollectionDescriptor(ComponentType t) const ;
    bool validateCollectionJson(const json& j, CollectionStructure s) const ;
};


#endif // CONTROL_API_HANDLER_HPP_
