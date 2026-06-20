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

#include "api/ControlApiHandler.hpp"
#include "api/DataApiHandler.hpp"
#include "core/BaseComponent.hpp"
#include "core/FileComponent.hpp"
#include "core/Engine.hpp"
#include "config/Config.hpp"
#include "configs/ComponentConfig.hpp"
#include "meta/CollectionDescriptor.hpp"
#include "meta/ComponentRegistry.hpp"
#include "requests/CollectionRequest.hpp"
#include "types/ParameterType.hpp"
#include "types/SocketType.hpp"

#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <fcntl.h>
#include <thread>
#include <optional>
#include <unordered_map>
#include <spdlog/spdlog.h>

ControlApiHandler::ControlApiHandler()
{}

ControlApiHandler* ControlApiHandler::instance(){
    static ControlApiHandler* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new ControlApiHandler();
    }
    return s_instance ;
}

void ControlApiHandler::initialize(Engine* engine){
    engine_ = engine ;

    // register api handler functions
    handlers_["get_audio_devices"] = [this](const json& request){ return getAudioDevices(request); };
    handlers_["set_audio_device"] = [this](const json& request){ return setAudioDevice(request); };
    handlers_["get_audio_configuration"] = [this](const json& request){ return getAudioConfig(request); };
    handlers_["get_midi_devices"] = [this](const json& request){ return getMidiDevices(request); };
    handlers_["set_midi_device"] = [this](const json& request){ return setMidiDevice(request); };
    handlers_["set_state"] = [this](const json& request){ return setState(request); };
    handlers_["get_configuration"] = [this](const json& request){ return getConfiguration(request); };
    handlers_["load_patch"] = [this](const json& request){ return loadPatch(request); };
    handlers_["add_component"] = [this](const json& request){ return addComponent(request); };
    handlers_["remove_component"] = [this](const json& request){ return removeComponent(request); };
    handlers_["create_connection"] = [this](const json& request){ return parseConnectionRequest(request); };
    handlers_["remove_connection"] = [this](const json& request){ return parseConnectionRequest(request); };
    handlers_["create_depth_connection"] = [this](const json& request){ return parseConnectionRequest(request); };
    handlers_["remove_depth_connection"] = [this](const json& request){ return parseConnectionRequest(request); };
    handlers_["sync_component"] = [this](const json& request){ return syncComponent(request); };
    handlers_["get_parameter"] = [this](const json& request){ return getParameter(request); };
    handlers_["set_parameter"] = [this](const json& request){ return setParameter(request); };
    handlers_["get_parameter_default"] = [this](const json& request){ return getParameterDefault(request); };
    handlers_["set_parameter_default"] = [this](const json& request){ return setParameterDefault(request); };
    handlers_["get_parameter_range"] = [this](const json& request){ return getParameterValueRange(request); };
    handlers_["set_parameter_range"] = [this](const json& request){ return setParameterValueRange(request); };
    handlers_["reset_parameter"] = [this](const json& request){ return resetParameter(request); };
    handlers_["add_collection_value"] = [this](const json& request){ return parseCollectionRequest(request); };
    handlers_["remove_collection_value"] = [this](const json& request){ return parseCollectionRequest(request); };
    handlers_["get_collection_value"] = [this](const json& request){ return parseCollectionRequest(request); };
    handlers_["get_collection_values"] = [this](const json& request){ return parseCollectionRequest(request); };
    handlers_["set_collection_value"] = [this](const json& request){ return parseCollectionRequest(request); };
    handlers_["reset_collection"] = [this](const json& request){ return parseCollectionRequest(request); };
    handlers_["get_collection_range"] = [this](const json& request){ return parseCollectionRequest(request); };
    handlers_["set_collection_range"] = [this](const json& request){ return parseCollectionRequest(request); };
    handlers_["get_modulation_strategy"] = [this](const json& request){ return getModulationStrategy(request); };
    handlers_["set_modulation_strategy"] = [this](const json& request){ return setModulationStrategy(request); };
    handlers_["get_modulation_depth"] = [this](const json& request){ return getModulationDepth(request); };
    handlers_["set_modulation_depth"] = [this](const json& request){ return setModulationDepth(request); };
    handlers_["get_file_path"] = [this](const json& request){ return getFilePath(request); };
    handlers_["set_file_path"] = [this](const json& request){ return setFilePath(request); };
    handlers_["get_buffer_data"] = [this](const json& request){ return getBufferData(request); };
}

void ControlApiHandler::start(){
    int serverPort = Config::get<int>("server.control_port").value() ;

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

    SPDLOG_INFO("Control API running on port {}... ",serverPort);

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
                ControlApiHandler::instance()->onClientConnection(sock);
            }).detach();  
        } else {
            // no pending connection; sleep briefly to avoid busy-wait
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    // Close the server socket
    close(serverSock);
}

void ControlApiHandler::onClientConnection(int sock){
    char buffer[1024] = {0};
    std::string partialData ;
    clientSockets_.insert(sock);

    while (!Engine::stop_flag){
        ssize_t bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0) ;
        if (bytesReceived > 0 ){
            buffer[bytesReceived] = '\0' ; // null-terminate the buffer so it can be read into a string
            partialData += buffer ; // read buffered text into data string

            // loop until we find a newline character
            size_t pos ;
            while ((pos = partialData.find('\n')) != std::string::npos ){
                // copy a complete message to our string to parse, then erase that portion from our data string.
                std::string jsonStr = partialData.substr(0,pos); 
                partialData.erase(0, pos + 1); 

                // handle the parsed json string
                handleClientMessage(jsonStr);
            }
        } else if (bytesReceived == 0){
            break ;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK ){
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

json ControlApiHandler::sendApiResponse(json& response, const std::string& err){
    if ( err == "" ){
        response["status"] = "success" ;
    } else {
        response["status"] = "failed" ;
        response["error"] = err ;
        SPDLOG_ERROR("Api Request Failed: {}", err);
    }
    std::string r = response.dump() + '\n' ;
    SPDLOG_INFO("sending API response: {}", r.c_str());
    for ( const auto& sock : clientSockets_ ){
        send(sock, r.c_str(), r.size(), 0);
    }
    return response ;
}


void ControlApiHandler::handleClientMessage(std::string jsonStr){
    json request;
    std::string action ;

    SPDLOG_INFO("received request: {}", jsonStr);

    try {
        request = json::parse(jsonStr);
        action = request["action"];
    } catch (const std::exception& e){
        sendApiResponse(request, "Error parsing json request: " + std::string(e.what()));
        return ;
    }
    
    auto it = handlers_.find(action);
    if ( it == handlers_.end() ){
        sendApiResponse(request, "unknown action requested: " + action );
        return ;
    }
    
    it->second(request);
}

const std::unordered_set<int>& ControlApiHandler::getOpenClientSockets() const {
    return clientSockets_ ;
}

json ControlApiHandler::getAudioDevices(const json& request){
    json response = request ;
    for ( const auto& dev : engine_->getAvailableAudioDevices() ){
        json j = {
            {"id", dev.ID},
            {"name", dev.name}
        };
        response["data"].push_back(j);
    }
    SPDLOG_DEBUG(response.dump());
    return sendApiResponse(response);
}

json ControlApiHandler::getMidiDevices(const json& request){
    json response = request ;
    for ( const auto& [id, name] : engine_->getAvailableMidiDevices() ){
        json j = {
            {"id", id},
            {"name", name}
        };
        response["data"].push_back(j);
    }
    return sendApiResponse(response);
}

json ControlApiHandler::setAudioDevice(const json& request){
    json response = request ;
    int deviceId ;
    std::string err ;
    
    try {
        deviceId = response.at("device_id");
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    if ( engine_->setAudioDeviceId(deviceId) ){
        auto output = sendApiResponse(response);
        if ( !engine_->isRunning() ){
            json j ;
            j["action"] = "get_audio_configuration" ;
            getAudioConfig(j);
        }
        return output ;
    } else {
        return sendApiResponse(response, "failed to set audio device");
    }
}

json ControlApiHandler::getAudioConfig(const json& request){
    json response = request ;

    response["device_id"] = engine_->getAudioDeviceId();
    response["output_channels"] = engine_->signalController.getNumChannels();

    return sendApiResponse(response);
}

json ControlApiHandler::setMidiDevice(const json& request){
    json response = request ;
    int deviceId ;
    
    try {
        deviceId = response.at("device_id");
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    if ( engine_->setMidiDeviceId(deviceId) ){
        return sendApiResponse(response);
    } else {
        return sendApiResponse(response, "failed to set midi device");
    }
}

json ControlApiHandler::setState(const json& request){
    json response = request ;
    std::string state ;

    try {
        state = response["state"];
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    if ( state == "run" ){
        engine_->run();
        return sendApiResponse(response);
    }

    if ( state == "stop" ){
        engine_->stop();
        return sendApiResponse(response);
    }

    return sendApiResponse(response, "Unrecognized engine state requested: " + state);
}

json ControlApiHandler::getConfiguration(const json& request){
    json response = request ;
    response["data"] = engine_->serialize();
    return sendApiResponse(response);
}

json ControlApiHandler::loadPatch(const json& request){
    json response = request ;
    
    // create components
    if ( 
        !response.contains("components") ||
        !response.at("components").is_array() 
    ){
        return sendApiResponse(response, "components array not properly defined");
    }

    std::unordered_map<int,int> idMap ; // old id, new id
    if ( ! loadCreateComponent(response["components"], idMap) ){
        return sendApiResponse(response, "Error creating components");
    }

    loadUpdateIds(response, idMap);

    // connect components
    if ( response.contains("connections") ){
        if ( !loadConnectComponent(response.at("connections")) ){
            return sendApiResponse(response, "Error connecting components");
        }
    }
    
    return sendApiResponse(response);
}

json ControlApiHandler::addComponent(const json& request){
    json response = request ;
    ComponentType type ;
    std::string name ;

    try {
        type = ComponentRegistry::getComponentDescriptor(response["name"].get<std::string>()).type;
        name = response["name"];
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    ComponentId id = engine_->componentFactory.createFromJson(type, name, getDefaultConfig(type));
    response["componentId"] = id;
    return sendApiResponse(response);
}

json ControlApiHandler::removeComponent(const json& request){
    json response = request ;
    ComponentId id ;

    try {
        id = response["componentId"];
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }
    
    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(response, "component not found.");
    }

    // query each subsystem and remove connections, if exist
    bool allRemoved = true ;

    auto connections = engine_->getComponentConnections(id);
    for ( auto c : connections ){
        c.remove = true ;
        json j = c ;
        SPDLOG_DEBUG("removing connection: {}", j.dump());
        auto cresponse = parseConnectionRequest(j);
        allRemoved = allRemoved && cresponse.contains("status") && cresponse["status"] == "success" ;
    }

    if ( !allRemoved ){
        return sendApiResponse(response, "at least one component connection could not be removed.");
    }

    engine_->componentManager.remove(id);
    return sendApiResponse(response);    
}

json ControlApiHandler::syncComponent(const json& request){
    json response = request ;

    int componentId ;
    try {
        componentId = response.at("componentId");
    } catch ( const std::exception& e ){
        return sendApiResponse(response, "required parameters were not provided.");
    }

    auto c = engine_->componentManager.getRaw(componentId);

    if ( !c ){
        return sendApiResponse(response, "could not find component with specified ID");
    }

    response["data"] = engine_->componentManager.serializeComponent(c);
    return sendApiResponse(response);
}

json ControlApiHandler::parseConnectionRequest(const json& request){
    json response = request ;
    ConnectionRequest req ;

    try {
        req = response.get<ConnectionRequest>() ;
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    if ( ! req.valid() ){
        return sendApiResponse(response, "Invalid connection request.");
    }

    if ( routeConnectionRequest(req)){
        return sendApiResponse(response);
    } else {
        return sendApiResponse(response, "failed to handle requested connection event");
    }
}

json ControlApiHandler::getParameter(const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param =  stringToParameter(response["parameter"]);
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(response, "Component not found");
    }

    response["value"] = c->getParameters()->getValueDispatch(param);
    return sendApiResponse(response);
}

json ControlApiHandler::setParameter(const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = stringToParameter(response["parameter"]);
        response.at("value"); // verify present
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(response, "Component not found");
    }

    bool setSuccess = c->getParameters()->setValueDispatch(param, response["value"]);
    response["value"] = c->getParameters()->getValueDispatch(param); // feed the value back to the client (due to limiting or other behaviors)
    if ( setSuccess ){
        return sendApiResponse(response);
    } else {
        return sendApiResponse(response, "Error setting component parameter." );
    }
}

json ControlApiHandler::getParameterDefault(const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = stringToParameter(response["parameter"]);
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(response, "Component not found");
    }

    response["value"] = c->getParameters()->getDefaultDispatch(param);
    return sendApiResponse(response);
}

json ControlApiHandler::setParameterDefault(const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = stringToParameter(response["parameter"]);
        response.at("value"); // verify present
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(response, "Component not found");
    }

    if ( c->getParameters()->setDefaultDispatch(param, response["value"]) ){
        return sendApiResponse(response);
    } else {
        return sendApiResponse(response, "Error setting component default." );
    }
}

json ControlApiHandler::getParameterValueRange(const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = stringToParameter(response["parameter"]);
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(response, "Component not found");
    }

    response["minimum"] = c->getParameters()->getMinDispatch(param);
    response["maximum"] = c->getParameters()->getMaxDispatch(param);

    return sendApiResponse(response);
}

json ControlApiHandler::setParameterValueRange(const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = stringToParameter(response["parameter"]);
        response.at("minimum"); 
        response.at("maximum"); 
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(response, "Component not found");
    }

    if ( !c->getParameters()->setMinDispatch(param, response["minimum"]) ){
        return sendApiResponse(response, "Error setting parameter minimum");
    }

    if ( !c->getParameters()->setMaxDispatch(param, response["maximum"]) ){
        return sendApiResponse(response, "Error setting parameter minimum");
    }

    return sendApiResponse(response);
}

json ControlApiHandler::resetParameter(const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = stringToParameter(response["parameter"]);
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(response, "Component not found");
    }

    response["minimum"] = c->getParameters()->getMinDispatch(param);
    response["maximum"] = c->getParameters()->getMaxDispatch(param);

    return sendApiResponse(response);
}

json ControlApiHandler::parseCollectionRequest(const json& request){
    json response = request ;
    ComponentId id ;
    CollectionType collectionType ;

    try {
        id = response["componentId"];
        collectionType = CollectionType(response["collection"]);
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(response, "Component not found");
    }

    const CollectionDescriptor* cd = nullptr ;
    CollectionRequest req ;
    try {
        cd = &getCollectionDescriptor(c->getType(), collectionType);
        req = response ;
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error getting collection: " + std::string(e.what()) );
    }

    if ( !cd || !cd->isValid() ){
        return sendApiResponse(response, "collection descriptor is malformed.");
    }

    if ( !req.valid(*cd) ){
        return sendApiResponse(response, "Invalid collection request structure");
    }

    switch(req.action){
    case CollectionAction::ADD:       return addCollectionValue(c, *cd, req);
    case CollectionAction::REMOVE:    return removeCollectionValue(c, *cd, req);
    case CollectionAction::GET:       return getCollectionValue(c, *cd, req);
    case CollectionAction::GET_RANGE: return getCollectionValueRange(c, *cd, req);
    case CollectionAction::SET:       return setCollectionValue(c, *cd, req);
    case CollectionAction::RESET:     return resetCollection(c, *cd, req);
    default:                          return sendApiResponse(response, "Unknown collection action");
    }
}

json ControlApiHandler::addCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request){
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            request.index = c->getParameters()->addCollectionValueDispatch(cd.params[0], request.value);
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                size_t idx = c->getParameters()->addCollectionValueDispatch(cd.params[0], (*request.value)[i]);
                if ( i == 0 ) request.index = idx ;
            }
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                request.index = c->getParameters()->addCollectionValueDispatch(
                    cd.params[i], 
                    (*request.value)[GET_PARAMETER_TRAIT_MEMBER(cd.params[i], name)]
                );
            }
            break ;
        default: 
        {
            json response = request ;
            sendApiResponse(response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(response, "failed to add collection value:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(response);

}

json ControlApiHandler::removeCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request){
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            c->getParameters()->removeCollectionValueDispatch(cd.params[0], *request.index);
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                c->getParameters()->removeCollectionValueDispatch(cd.params[0], *request.index);
            }
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                c->getParameters()->removeCollectionValueDispatch(cd.params[i], *request.index);
            }
            break ;
        default: 
        {
            json response = request ;
            sendApiResponse(response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(response, "failed to remove collection value:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(response);
}

json ControlApiHandler::getCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request){
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            request.value = c->getParameters()->getCollectionValueDispatch(cd.params[0], *request.index);
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                (*request.value)[i] = c->getParameters()->getCollectionValueDispatch(cd.params[0], *request.index);
            }
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                (*request.value)[GET_PARAMETER_TRAIT_MEMBER(cd.params[i], name)] = 
                    c->getParameters()->getCollectionValueDispatch(cd.params[i], *request.index);
            }
            break ;
        default: 
        {
            json response = request ;
            sendApiResponse(response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(response, "failed to get collection value:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(response);
}

json ControlApiHandler::setCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request){
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            c->getParameters()->setCollectionValueDispatch(cd.params[0], *request.index, *request.value);
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                c->getParameters()->setCollectionValueDispatch(cd.params[0], *request.index + i, (*request.value)[i]);
            }
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                c->getParameters()->setCollectionValueDispatch(cd.params[i], *request.index, 
                    (*request.value)[GET_PARAMETER_TRAIT_MEMBER(cd.params[i], name)]);
            }
            break ;
        default: 
        {
            json response = request ;
            sendApiResponse(response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(response, "failed to set collection values:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(response);
}

json ControlApiHandler::resetCollection(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request){
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            c->getParameters()->resetCollectionDispatch(cd.params[0]);
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                c->getParameters()->resetCollectionDispatch(cd.params[0]);
            }
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                c->getParameters()->resetCollectionDispatch(cd.params[i]);
            }
            break ;
        default: 
        {
            json response = request ;
            sendApiResponse(response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(response, "failed to reset collection:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(response);
}

json ControlApiHandler::getCollectionValueRange(BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request){
    json min ;
    json max ;
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
        case CollectionStructure::GROUPED:
            (*request.value)[0] = c->getParameters()->getCollectionMinDispatch(cd.params[0]);
            (*request.value)[1] = c->getParameters()->getCollectionMaxDispatch( cd.params[0]);
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                (*request.value)[GET_PARAMETER_TRAIT_MEMBER(cd.params[i], name)][0] = 
                    c->getParameters()->getCollectionMinDispatch(cd.params[i]);
                (*request.value)[GET_PARAMETER_TRAIT_MEMBER(cd.params[i], name)][1] = 
                    c->getParameters()->getCollectionMinDispatch(cd.params[i]);
            }
            break ;
        default: 
        {
            json response = request ;
            sendApiResponse(response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(response, "failed to get collection range:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(response);
}

json ControlApiHandler::getModulationStrategy(const json& request){
    ComponentId id ;
    ParameterType p ;
    ModulationStrategy s ;
    json response = request ;

    try {
        id = request["componentId"];
        p = stringToParameter(request["parameter"]);
    } catch (const std::exception& e){
        json response = request ;
        return sendApiResponse(response, 
            "failed to parse request to update modulation strategy:" + std::string(e.what())
        );
    }

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        json response = request ;
        return sendApiResponse(response,
            "could not find component"
        );
    }

    const std::vector<ParameterType>&  modulatable = ComponentRegistry::getComponentDescriptor(
        component->getType()).modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        return sendApiResponse(response,
            "Parameter " + std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)) + " is not listed as modulatable for this component."
        );
    }

    try {
        s = component->getParameterModulationStrategy(p);
    } catch ( const std::exception& e ){
        return sendApiResponse(response,
            "Failed to get strategy from specified parameter: " + std::string(e.what())
        );
    }
    
    response["strategy"] = s ;
    return sendApiResponse(response);
}

json ControlApiHandler::setModulationStrategy(const json& request){
    ComponentId id ;
    ParameterType p ;
    ModulationStrategy s ;
    json response = request ;

    try {
        id = request["componentId"];
        p = stringToParameter(request["parameter"]);
        s = static_cast<ModulationStrategy>(request["strategy"]);
    } catch (const std::exception& e){
        json response = request ;
        return sendApiResponse(response, 
            "failed to parse request to update modulation strategy:" + std::string(e.what())
        );
    }

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        json response = request ;
        return sendApiResponse(response,
            "could not find component"
        );
    }

    const std::vector<ParameterType>&  modulatable = ComponentRegistry::getComponentDescriptor(
        component->getType()).modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        return sendApiResponse(response,
            "Parameter " + std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)) + " is not listed as modulatable for this component."
        );
    }

    component->setParameterModulationStrategy(p, s);
    return sendApiResponse(response);
}

json ControlApiHandler::getModulationDepth(const json& request){
    ComponentId id ;
    ParameterType p ;
    double depth ;
    json response = request ;

    try {
        id = request["componentId"];
        p = stringToParameter(request["parameter"]);
    } catch (const std::exception& e){
        json response = request ;
        return sendApiResponse(response, 
            "failed to parse request to update modulation strategy:" + std::string(e.what())
        );
    }

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        json response = request ;
        return sendApiResponse(response,
            "could not find component"
        );
    }
    
    const std::vector<ParameterType>&  modulatable = ComponentRegistry::getComponentDescriptor(
        component->getType()).modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        return sendApiResponse(response,
            "Parameter " + std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)) + " is not listed as modulatable for this component."
        );
    }

    try {
        depth = component->getParameterDepth(p);
    } catch (const std::exception& e){
        return sendApiResponse(response,
            "Failed to get depth from specified parameter: " + std::string(e.what())
        );
    }
    
    response["depth"] = depth ;
    return sendApiResponse(response);
}


json ControlApiHandler::setModulationDepth(const json& request){
    ComponentId id ;
    ParameterType p ;
    double depth ;
    json response = request ;

    try {
        id = request["componentId"];
        p = stringToParameter(request["parameter"]);
        depth = request["depth"];
    } catch (const std::exception& e){
        json response = request ;
        return sendApiResponse(response, 
            "failed to parse request to update modulation strategy:" + std::string(e.what())
        );
    }

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        json response = request ;
        return sendApiResponse(response,
            "could not find component"
        );
    }
    
    const std::vector<ParameterType>&  modulatable = ComponentRegistry::getComponentDescriptor(
        component->getType()).modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        return sendApiResponse(response,
            "Parameter " + std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)) + " is not listed as modulatable for this component."
        );
    }

    component->setParameterDepth(p, depth);
    return sendApiResponse(response);
}

json ControlApiHandler::getFilePath(const json& request){
    json response = request ;
    ComponentId id ;
    std::string path ;

    try {
        id = response.at("componentId");
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    FileComponent* c = engine_->componentManager.getFileComponent(id);
    if ( !c ){
        return sendApiResponse(response, "File component not found");
    }

    path = c->getPath();
    response["path"] = path ;

    return sendApiResponse(response);
}

json ControlApiHandler::setFilePath(const json& request){
    json response = request ;
    ComponentId id ;
    std::string path ;

    try {
        id = response.at("componentId");
        path = response.at("path");
    } catch (const std::exception& e){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()) );
    }

    FileComponent* c = engine_->componentManager.getFileComponent(id);
    if ( !c ){
        return sendApiResponse(response, "File component not found");
    }

    c->setPath(path);
    return sendApiResponse(response);
}

json ControlApiHandler::getBufferData(const json& request){
    json response = request ;
    ComponentId id ;
    size_t channel ;

    try {
        id = response.at("componentId");
        channel = response.at("channel");
    } catch ( const std::exception& e ){
        return sendApiResponse(response, "Error parsing json request: " + std::string(e.what()));
    }

    AudioBufferComponent* c = engine_->componentManager.getBufferComponent(id);
    if ( !c ){
        return sendApiResponse(response, "Buffer component not found");
    }

    if ( channel >= c->getNumOutputs() ){
        return sendApiResponse(response, "Invalid channel number for buffer component");
    }

    const auto& buffer = c->getBuffer(channel);

    DataDescriptor header {
        .componentId = static_cast<uint32_t>(id),
        .channel = static_cast<uint32_t>(channel),
        .size = static_cast<uint64_t>(buffer.size() * sizeof(double))
    };

    DataApiHandler::instance()->sendApiData(header, c->getBuffer(channel));
    return sendApiResponse(response);

}

bool ControlApiHandler::routeConnectionRequest(ConnectionRequest request){
    if ( request.inboundSocket == SocketType::MidiInbound && request.outboundSocket == SocketType::MidiOutbound )
        return engine_->handleMidiConnection(request);
    if ( request.inboundSocket == SocketType::SignalInbound && request.outboundSocket == SocketType::SignalOutbound )
        return engine_->handleSignalConnection(request);
    if ( request.inboundSocket == SocketType::BufferInbound && request.outboundSocket == SocketType::BufferOutbound )
        return engine_->handleBufferConnection(request);
    if ( request.inboundSocket == SocketType::ModulationInbound && request.outboundSocket == SocketType::ModulationOutbound )
        return engine_->handleModulationConnection(request);

    SPDLOG_WARN("WARN: socket params are incompatible. No connection will be made");
    return false ;
}

bool ControlApiHandler::loadCreateComponent(const json& components, std::unordered_map<int,int>& idMap){
    json params ;
    ComponentId id ;
    json componentRequest ;
    json componentResponse ;
    json parameterRequest ;

    bool success = true ;
    for ( const auto& component : components ){
        try {
            params = component.at("parameters");
            id = component.at("id");
            componentRequest["action"] = "add_component" ;
            componentRequest["name"] = component.at("name") ;
            componentResponse = addComponent(componentRequest);
            idMap[id] = componentResponse["componentId"];

            for ( const auto& [p, data] : params.items() ){
                if ( ! data.contains("currentValue") ){
                    continue ; // just has modulation
                }
                parameterRequest["action"] = "set_component_parameter" ;
                parameterRequest["componentId"] = idMap[id] ;
                parameterRequest["parameter"] = p ;
                parameterRequest["value"] = data.at("currentValue") ;
                setParameter(parameterRequest);
            }
        } catch ( const std::exception& e ){
            SPDLOG_WARN("Error creating component: {}", std::string(e.what()));
            success = false ;
        }
    }

    return success ;
}

bool ControlApiHandler::loadConnectComponent(const json& connections){
    bool success = true ;

    if ( !connections.is_array() ) return false ;

    for ( auto c : connections ){
        ConnectionRequest request ;
        try {
            request = c ;
        } catch (std::exception& e){
            SPDLOG_ERROR("Could not parse {} to ConnectionRequest.", c.dump());
            success = false ;
        }

        const auto& connectionResponse = parseConnectionRequest(request);
        if ( ! connectionResponse.contains("status") || connectionResponse.at("status") != "success" ){
            SPDLOG_ERROR("error requesting connection: {}", connectionResponse.dump());
            success = false ;
        }
    }

    return success ;

}

void ControlApiHandler::loadUpdateIds(json& j, const std::unordered_map<int, int>& idMap){
    if ( j.is_object() ) {
        // Update known ID fields in objects
        std::vector<std::string> idKeys = {"id", "ComponentId", "componentId", "modulatorId"};
        for ( const auto& key : idKeys ) {
            if ( j.contains(key) && j[key].is_number_integer() ) {
                int currentId = j[key];
                auto it = idMap.find(currentId);
                if ( it != idMap.end() ) {
                    j[key] = it->second; 
                }
            }
        }
        
        // Handle known arrays that contain bare IDs
        std::vector<std::string> idArrayKeys = {"MidiHandlers", "midiListeners", "MidiListeners", "componentIds"};
        for ( const auto& key : idArrayKeys ) {
            if ( j.contains(key) && j[key].is_array() ) {
                for (auto& element : j[key]) {
                    if (element.is_number_integer()) {
                        int currentId = element;
                        auto it = idMap.find(currentId);
                        if (it != idMap.end()) {
                            element = it->second;
                        }
                    }
                }
            }
        }
        
        // Recurse into all values
        for ( auto& [key, value] : j.items() ) {
            loadUpdateIds(value, idMap);
        }
    }
    else if ( j.is_array() ) {
        // Recurse into array elements
        for (auto& element : j) {
            loadUpdateIds(element, idMap);
        }
    }
}

const CollectionDescriptor& ControlApiHandler::getCollectionDescriptor(ComponentType t, CollectionType c) const {
    const ComponentDescriptor& descriptor = ComponentRegistry::getComponentDescriptor(t);
    int idx = descriptor.hasCollection(c);
    if ( idx == -1 ){
        std::string msg = fmt::format(
            "Cannot retrieve collection {} from Component Type {}.", 
            CollectionType::toString(c),  
            ComponentRegistry::getComponentDescriptor(t).name
        );
        SPDLOG_ERROR(msg);
        throw std::runtime_error(msg);
    }

    return descriptor.getCollection(idx);
}