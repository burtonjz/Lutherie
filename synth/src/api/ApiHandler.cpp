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

#include "api/ApiHandler.hpp"
#include "core/BaseComponent.hpp"
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

ApiHandler::ApiHandler()
{}

ApiHandler* ApiHandler::instance(){
    static ApiHandler* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new ApiHandler();
    }
    return s_instance ;
}

void ApiHandler::initialize(Engine* engine){
    engine_ = engine ;

    // register api handler functions
    handlers_["get_audio_devices"] = [this](int sock, const json& request){ return getAudioDevices(sock, request); };
    handlers_["get_midi_devices"] = [this](int sock, const json& request){ return getMidiDevices(sock, request); };
    handlers_["set_audio_device"] = [this](int sock, const json& request){ return setAudioDevice(sock, request); };
    handlers_["set_midi_device"] = [this](int sock, const json& request){ return setMidiDevice(sock, request); };
    handlers_["set_state"] = [this](int sock, const json& request){ return setState(sock, request); };
    handlers_["get_configuration"] = [this](int sock, const json& request){ return getConfiguration(sock, request); };
    handlers_["load_patch"] = [this](int sock, const json& request){ return loadPatch(sock, request); };
    handlers_["add_component"] = [this](int sock, const json& request){ return addComponent(sock, request); };
    handlers_["remove_component"] = [this](int sock, const json& request){ return removeComponent(sock, request); };
    handlers_["create_connection"] = [this](int sock, const json& request){ return parseConnectionRequest(sock, request); };
    handlers_["remove_connection"] = [this](int sock, const json& request){ return parseConnectionRequest(sock, request); };
    handlers_["create_depth_connection"] = [this](int sock, const json& request){ return parseConnectionRequest(sock, request); };
    handlers_["remove_depth_connection"] = [this](int sock, const json& request){ return parseConnectionRequest(sock, request); };
    handlers_["sync_component"] = [this](int sock, const json& request){ return syncComponent(sock, request); };
    handlers_["get_parameter"] = [this](int sock, const json& request){ return getParameter(sock, request); };
    handlers_["set_parameter"] = [this](int sock, const json& request){ return setParameter(sock, request); };
    handlers_["get_parameter_default"] = [this](int sock, const json& request){ return getParameterDefault(sock, request); };
    handlers_["set_parameter_default"] = [this](int sock, const json& request){ return setParameterDefault(sock, request); };
    handlers_["get_parameter_range"] = [this](int sock, const json& request){ return getParameterValueRange(sock, request); };
    handlers_["set_parameter_range"] = [this](int sock, const json& request){ return setParameterValueRange(sock, request); };
    handlers_["reset_parameter"] = [this](int sock, const json& request){ return resetParameter(sock, request); };
    handlers_["add_collection_value"] = [this](int sock, const json& request){ return parseCollectionRequest(sock, request); };
    handlers_["remove_collection_value"] = [this](int sock, const json& request){ return parseCollectionRequest(sock, request); };
    handlers_["get_collection_value"] = [this](int sock, const json& request){ return parseCollectionRequest(sock, request); };
    handlers_["get_collection_values"] = [this](int sock, const json& request){ return parseCollectionRequest(sock, request); };
    handlers_["set_collection_value"] = [this](int sock, const json& request){ return parseCollectionRequest(sock, request); };
    handlers_["reset_collection"] = [this](int sock, const json& request){ return parseCollectionRequest(sock, request); };
    handlers_["get_collection_range"] = [this](int sock, const json& request){ return parseCollectionRequest(sock, request); };
    handlers_["set_collection_range"] = [this](int sock, const json& request){ return parseCollectionRequest(sock, request); };
    handlers_["get_modulation_strategy"] = [this](int sock, const json& request){ return getModulationStrategy(sock, request); };
    handlers_["set_modulation_strategy"] = [this](int sock, const json& request){ return setModulationStrategy(sock, request); };
    handlers_["get_modulation_depth"] = [this](int sock, const json& request){ return getModulationDepth(sock, request); };
    handlers_["set_modulation_depth"] = [this](int sock, const json& request){ return setModulationDepth(sock, request); };
}

void ApiHandler::start(){
    int serverPort = Config::get<int>("server.port").value() ;

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

    SPDLOG_INFO("Server listening on port {}... ",serverPort);

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
                ApiHandler::instance()->onClientConnection(sock);
            }).detach();  
        } else {
            // no pending connection; sleep briefly to avoid busy-wait
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    // Close the server socket
    close(serverSock);
}

void ApiHandler::onClientConnection(int sock){
    char buffer[1024] = {0};
    std::string partialData ;

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
                handleClientMessage(sock, jsonStr);
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

    close(sock);
}

json ApiHandler::sendApiResponse(int sock, json& response, const std::string& err){
    if ( err == "" ){
        response["status"] = "success" ;
    } else {
        response["status"] = "failed" ;
        response["error"] = err ;
        SPDLOG_ERROR("Api Request Failed: {}", err);
    }
    std::string r = response.dump() + '\n' ;
    SPDLOG_INFO("sending API response: {}", r.c_str());
    send(sock, r.c_str(), r.size(), 0);
    return response ;
}


void ApiHandler::handleClientMessage(int sock, std::string jsonStr){
    json request;
    std::string action ;

    SPDLOG_INFO("received request: {}", jsonStr);

    try {
        request = json::parse(jsonStr);
        action = request["action"];
    } catch (const std::exception& e){
        sendApiResponse(sock,request, "Error parsing json request: " + std::string(e.what()));
        return ;
    }
    
    auto it = handlers_.find(action);
    if ( it == handlers_.end() ){
        sendApiResponse(sock, request, "unknown action requested: " + action );
        return ;
    }
    
    it->second(sock, request);
}

json ApiHandler::getAudioDevices(int sock, const json& request){
    json response = request ;
    response["data"] = engine_->getAvailableAudioDevices() ;
    return sendApiResponse(sock,response);
}

json ApiHandler::getMidiDevices(int sock, const json& request){
    json response = request ;
    response["data"] = engine_->getAvailableMidiDevices() ;
    return sendApiResponse(sock,response);
}

json ApiHandler::setAudioDevice(int sock, const json& request){
    json response = request ;
    int deviceId ;
    std::string err ;
    
    try {
        deviceId = response["device_id"];
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    if ( engine_->setAudioDeviceId(deviceId) ){
        return sendApiResponse(sock,response);
    } else {
        return sendApiResponse(sock, response, "failed to set audio device");
    }
}

json ApiHandler::setMidiDevice(int sock, const json& request){
    json response = request ;
    int deviceId ;
    
    try {
        deviceId = response["device_id"];
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    if ( engine_->setMidiDeviceId(deviceId) ){
        return sendApiResponse(sock,response);
    } else {
        return sendApiResponse(sock, response, "failed to set midi device");
    }
}

json ApiHandler::setState(int sock, const json& request){
    json response = request ;
    std::string state ;

    try {
        state = response["state"];
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    if ( state == "run" ){
        engine_->run();
        return sendApiResponse(sock,response);
    }

    if ( state == "stop" ){
        engine_->stop();
        return sendApiResponse(sock,response);
    }

    return sendApiResponse(sock,response, "Unrecognized engine state requested: " + state);
}

json ApiHandler::getConfiguration(int sock, const json& request){
    json response = request ;
    response["data"] = engine_->serialize();
    return sendApiResponse(sock,response);
}

json ApiHandler::loadPatch(int sock, const json& request){
    json response = request ;
    
    // create components
    try {
        response.at("components");
        assert(response["components"].is_array() && "components is not a json array");
    } catch ( const std::exception& e ){
        return sendApiResponse(sock, response, "Error processing json request " + std::string(e.what()));
    }

    std::unordered_map<int,int> idMap ;
    if ( ! loadCreateComponent(sock,response["components"], idMap) ){
        return sendApiResponse(sock, response, "Error creating components");
    }

    // update response data with new component ids
    loadUpdateIds(response, idMap);

    // connect components
    if ( ! loadConnectComponent(sock, response) ){
        return sendApiResponse(sock, response, "Error connecting components");
    }

    return sendApiResponse(sock, response);
}

json ApiHandler::addComponent(int sock, const json& request){
    json response = request ;
    ComponentType type ;
    std::string name ;

    try {
        type = static_cast<ComponentType>(response["type"]);
        name = response["name"];
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    ComponentId id = engine_->componentFactory.createFromJson(type, name, getDefaultConfig(type));
    response["componentId"] = id;
    return sendApiResponse(sock,response);
}

json ApiHandler::removeComponent(int sock, const json& request){
    json response = request ;
    ComponentId id ;

    try {
        id = response["componentId"];
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }
    
    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(sock, response, "component not found.");
    }

    // query each subsystem and remove connections, if exist
    bool allRemoved = true ;

    auto connections = engine_->getComponentMidiConnections(id);
    SPDLOG_DEBUG("removing midi connections from component with id {}", id);
    for ( auto c : connections ){
        c.remove = true ;
        json j = c ;
        SPDLOG_DEBUG("removing midi connection: {}", j.dump());
        auto cresponse = parseConnectionRequest(sock, j);
        allRemoved = allRemoved && cresponse.contains("status") && cresponse["status"] == "success" ;
    }

    connections = engine_->getComponentSignalConnections(id);
    SPDLOG_DEBUG("removing audio connections from component with id {}", id);
    for ( auto c : connections ){
        c.remove = true ;
        json j = c ;
        SPDLOG_DEBUG("removing audio connection: {}", j.dump());
        auto cresponse = parseConnectionRequest(sock, j);
        allRemoved = allRemoved && cresponse.contains("status") && cresponse["status"] == "success" ;
    }

    
    connections = engine_->getComponentModulationConnections(id);
    SPDLOG_DEBUG("removing modulation connections from component with id {}", id);
    for ( auto c : connections ){
        c.remove = true ;
        json j = c ;
        SPDLOG_DEBUG("removing modulation connection: {}", j.dump());
        auto cresponse = parseConnectionRequest(sock, j);
        allRemoved = allRemoved && cresponse.contains("status") && cresponse["status"] == "success" ;
    }

    if ( !allRemoved ){
        return sendApiResponse(sock, response, "at least one component connection could not be removed.");
    }

    engine_->componentManager.remove(id);
    return sendApiResponse(sock, response);    
}

json ApiHandler::syncComponent(int sock, const json& request){
    json response = request ;

    int componentId ;
    try {
        componentId = response.at("componentId");
    } catch ( const std::exception& e ){
        return sendApiResponse(sock, response, "required parameters were not provided.");
    }

    auto c = engine_->componentManager.getRaw(componentId);

    if ( !c ){
        return sendApiResponse(sock, response, "could not find component with specified ID");
    }

    response["data"] = engine_->componentManager.serializeComponent(c);
    return sendApiResponse(sock, response);
}

json ApiHandler::parseConnectionRequest(int sock, const json& request){
    json response = request ;
    ConnectionRequest req ;

    try {
        req = response.get<ConnectionRequest>() ;
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    if ( ! req.valid() ){
        return sendApiResponse(sock, response, "Invalid connection request.");
    }

    if ( routeConnectionRequest(req)){
        return sendApiResponse(sock,response);
    } else {
        return sendApiResponse(sock,response, "failed to handle requested connection event");
    }
}

json ApiHandler::getParameter(int sock, const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = static_cast<ParameterType>(response["parameter"]);
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(sock, response, "Component not found");
    }

    response["value"] = c->getParameters()->getValueDispatch(param);
    return sendApiResponse(sock, response);
}

json ApiHandler::setParameter(int sock, const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = static_cast<ParameterType>(response["parameter"]);
        response.at("value"); // verify present
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(sock, response, "Component not found");
    }

    bool setSuccess = c->getParameters()->setValueDispatch(param, response["value"]);
    response["value"] = c->getParameters()->getValueDispatch(param); // feed the value back to the client (due to limiting or other behaviors)
    if ( setSuccess ){
        return sendApiResponse(sock,response);
    } else {
        return sendApiResponse(sock,response, "Error setting component parameter." );
    }
}

json ApiHandler::getParameterDefault(int sock, const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = static_cast<ParameterType>(response["parameter"]);
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(sock, response, "Component not found");
    }

    response["value"] = c->getParameters()->getDefaultDispatch(param);
    return sendApiResponse(sock, response);
}

json ApiHandler::setParameterDefault(int sock, const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = static_cast<ParameterType>(response["parameter"]);
        response.at("value"); // verify present
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(sock, response, "Component not found");
    }

    if ( c->getParameters()->setDefaultDispatch(param, response["value"]) ){
        return sendApiResponse(sock,response);
    } else {
        return sendApiResponse(sock,response, "Error setting component default." );
    }
}

json ApiHandler::getParameterValueRange(int sock, const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = static_cast<ParameterType>(response["parameter"]);
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(sock, response, "Component not found");
    }

    response["minimum"] = c->getParameters()->getMinDispatch(param);
    response["maximum"] = c->getParameters()->getMaxDispatch(param);

    return sendApiResponse(sock, response);
}

json ApiHandler::setParameterValueRange(int sock, const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = static_cast<ParameterType>(response["parameter"]);
        response.at("minimum"); 
        response.at("maximum"); 
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(sock, response, "Component not found");
    }

    if ( !c->getParameters()->setMinDispatch(param, response["minimum"]) ){
        return sendApiResponse(sock,response, "Error setting parameter minimum");
    }

    if ( !c->getParameters()->setMaxDispatch(param, response["maximum"]) ){
        return sendApiResponse(sock,response, "Error setting parameter minimum");
    }

    return sendApiResponse(sock, response);
}

json ApiHandler::resetParameter(int sock, const json& request){
    json response = request ;
    ComponentId id ;
    ParameterType param ;

    try {
        id = response["componentId"];
        param = static_cast<ParameterType>(response["parameter"]);
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(sock, response, "Component not found");
    }

    response["minimum"] = c->getParameters()->getMinDispatch(param);
    response["maximum"] = c->getParameters()->getMaxDispatch(param);

    return sendApiResponse(sock, response);
}

json ApiHandler::parseCollectionRequest(int sock, const json& request){
    json response = request ;
    ComponentId id ;
    CollectionType collectionType ;

    try {
        id = response["componentId"];
        collectionType = CollectionType(response["collection"]);
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error parsing json request: " + std::string(e.what()) );
    }

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        return sendApiResponse(sock, response, "Component not found");
    }

    const CollectionDescriptor* cd = nullptr ;
    CollectionRequest req ;
    try {
        cd = &getCollectionDescriptor(c->getType(), collectionType);
        req = response ;
    } catch (const std::exception& e){
        return sendApiResponse(sock,response, "Error getting collection: " + std::string(e.what()) );
    }

    if ( !cd || !cd->isValid() ){
        return sendApiResponse(sock, response, "collection descriptor is malformed.");
    }

    if ( !req.valid(*cd) ){
        return sendApiResponse(sock, response, "Invalid collection request structure");
    }

    switch(req.action){
    case CollectionAction::ADD:       return addCollectionValue(sock, c, *cd, req);
    case CollectionAction::REMOVE:    return removeCollectionValue(sock, c, *cd, req);
    case CollectionAction::GET:       return getCollectionValue(sock, c, *cd, req);
    case CollectionAction::GET_RANGE: return getCollectionValueRange(sock, c, *cd, req);
    case CollectionAction::SET:       return setCollectionValue(sock, c, *cd, req);
    case CollectionAction::RESET:     return resetCollection(sock, c, *cd, req);
    default:                          return sendApiResponse(sock, response, "Unknown collection action");
    }
}

json ApiHandler::addCollectionValue(int sock, BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request){
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
            sendApiResponse(sock, response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(sock, response, "failed to add collection value:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(sock, response);

}

json ApiHandler::removeCollectionValue(int sock, BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request){
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
            sendApiResponse(sock, response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(sock, response, "failed to remove collection value:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(sock, response);
}

json ApiHandler::getCollectionValue(int sock, BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request){
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
            sendApiResponse(sock, response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(sock, response, "failed to get collection value:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(sock, response);
}

json ApiHandler::setCollectionValue(int sock, BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request){
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
            sendApiResponse(sock, response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(sock, response, "failed to set collection values:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(sock, response);
}

json ApiHandler::resetCollection(int sock, BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request){
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
            sendApiResponse(sock, response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(sock, response, "failed to reset collection:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(sock, response);
}

json ApiHandler::getCollectionValueRange(int sock, BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request){
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
            sendApiResponse(sock, response, "unrecognized collection structure");
            break ;
        }}
    } catch (const std::exception& e){
        json response = request ;
        sendApiResponse(sock, response, "failed to get collection range:" + std::string(e.what()));
    }

    json response = request ;
    return sendApiResponse(sock, response);
}

json ApiHandler::getModulationStrategy(int sock, const json& request){
    ComponentId id ;
    ParameterType p ;
    ModulationStrategy s ;
    json response = request ;

    try {
        id = request["componentId"];
        p = parameterFromString(request["parameter"]);
    } catch (const std::exception& e){
        json response = request ;
        return sendApiResponse(sock, response, 
            "failed to parse request to update modulation strategy:" + std::string(e.what())
        );
    }

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        json response = request ;
        return sendApiResponse(sock, response,
            "could not find component"
        );
    }

    const std::vector<ParameterType>&  modulatable = ComponentRegistry::getComponentDescriptor(
        component->getType()).modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        return sendApiResponse(sock, response,
            "Parameter " + GET_PARAMETER_TRAIT_MEMBER(p, name) + " is not listed as modulatable for this component."
        );
    }

    try {
        s = component->getParameterModulationStrategy(p);
    } catch ( const std::exception& e ){
        return sendApiResponse(sock, response,
            "Failed to get strategy from specified parameter: " + std::string(e.what())
        );
    }
    
    response["strategy"] = s ;
    return sendApiResponse(sock, response);
}

json ApiHandler::setModulationStrategy(int sock, const json& request){
    ComponentId id ;
    ParameterType p ;
    ModulationStrategy s ;
    json response = request ;

    try {
        id = request["componentId"];
        p = parameterFromString(request["parameter"]);
        s = static_cast<ModulationStrategy>(request["strategy"]);
    } catch (const std::exception& e){
        json response = request ;
        return sendApiResponse(sock, response, 
            "failed to parse request to update modulation strategy:" + std::string(e.what())
        );
    }

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        json response = request ;
        return sendApiResponse(sock, response,
            "could not find component"
        );
    }

    const std::vector<ParameterType>&  modulatable = ComponentRegistry::getComponentDescriptor(
        component->getType()).modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        return sendApiResponse(sock, response,
            "Parameter " + GET_PARAMETER_TRAIT_MEMBER(p, name) + " is not listed as modulatable for this component."
        );
    }

    component->setParameterModulationStrategy(p, s);
    return sendApiResponse(sock, response);
}

json ApiHandler::getModulationDepth(int sock, const json& request){
    ComponentId id ;
    ParameterType p ;
    double depth ;
    json response = request ;

    try {
        id = request["componentId"];
        p = parameterFromString(request["parameter"]);
    } catch (const std::exception& e){
        json response = request ;
        return sendApiResponse(sock, response, 
            "failed to parse request to update modulation strategy:" + std::string(e.what())
        );
    }

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        json response = request ;
        return sendApiResponse(sock, response,
            "could not find component"
        );
    }
    
    const std::vector<ParameterType>&  modulatable = ComponentRegistry::getComponentDescriptor(
        component->getType()).modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        return sendApiResponse(sock, response,
            "Parameter " + GET_PARAMETER_TRAIT_MEMBER(p, name) + " is not listed as modulatable for this component."
        );
    }

    try {
        depth = component->getParameterDepth(p);
    } catch (const std::exception& e){
        return sendApiResponse(sock, response,
            "Failed to get depth from specified parameter: " + std::string(e.what())
        );
    }
    
    response["depth"] = depth ;
    return sendApiResponse(sock, response);
}


json ApiHandler::setModulationDepth(int sock, const json& request){
    ComponentId id ;
    ParameterType p ;
    double depth ;
    json response = request ;

    try {
        id = request["componentId"];
        p = parameterFromString(request["parameter"]);
        depth = request["depth"];
    } catch (const std::exception& e){
        json response = request ;
        return sendApiResponse(sock, response, 
            "failed to parse request to update modulation strategy:" + std::string(e.what())
        );
    }

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        json response = request ;
        return sendApiResponse(sock, response,
            "could not find component"
        );
    }
    
    const std::vector<ParameterType>&  modulatable = ComponentRegistry::getComponentDescriptor(
        component->getType()).modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        return sendApiResponse(sock, response,
            "Parameter " + GET_PARAMETER_TRAIT_MEMBER(p, name) + " is not listed as modulatable for this component."
        );
    }

    component->setParameterDepth(p, depth);
    return sendApiResponse(sock, response);
}

bool ApiHandler::routeConnectionRequest(ConnectionRequest request){
    if ( request.inboundSocket == SocketType::MidiInbound && request.outboundSocket == SocketType::MidiOutbound )
        return engine_->handleMidiConnection(request);
    if ( request.inboundSocket == SocketType::SignalInbound && request.outboundSocket == SocketType::SignalOutbound )
        return engine_->handleSignalConnection(request);

    if ( request.inboundSocket == SocketType::ModulationInbound && request.outboundSocket == SocketType::ModulationOutbound )
        return engine_->handleModulationConnection(request);

    SPDLOG_WARN("WARN: socket params are incompatible. No connection will be made");
    return false ;
}

bool ApiHandler::loadCreateComponent(int sock, const json& components, std::unordered_map<int,int>& idMap){
    json params ;
    ComponentId id ;
    ComponentType type ;
    json componentRequest ;
    json componentResponse ;
    json parameterRequest ;
    ParameterType parameterType ;

    bool success = true ;
    for ( const auto& component : components ){
        try {
            params = component.at("parameters");
            id = component.at("id");
            type = static_cast<ComponentType>(component.at("type"));
            componentRequest["action"] = "add_component" ;
            componentRequest["name"] = ComponentRegistry::getComponentDescriptor(type).name ;
            componentRequest["type"] = static_cast<int>(type) ;
            componentResponse = addComponent(sock, componentRequest);
            idMap[id] = componentResponse["componentId"];

            for ( const auto& [p, data] : params.items() ){
                if ( ! data.contains("currentValue") ){
                    continue ; // just has modulation
                }
                parameterType = static_cast<ParameterType>(parameterFromString(p));
                parameterRequest["action"] = "set_component_parameter" ;
                parameterRequest["componentId"] = idMap[id] ;
                parameterRequest["parameter"] = static_cast<int>(parameterType);
                parameterRequest["value"] = data.at("currentValue") ;
                setParameter(sock, parameterRequest);
            }
        } catch ( const std::exception& e ){
            SPDLOG_WARN("Error creating component: {}", std::string(e.what()));
            success = false ;
        }
    }

    return success ;
}

bool ApiHandler::loadConnectComponent(int sock, const json& config){
    bool success = true ;

    // connections to outbound audio device
    if ( config.contains("AudioSinks") && config.at("AudioSinks").is_array() ){
        for ( const auto& sink : config.at("AudioSinks") ){
            if ( 
                ! sink.contains("componentId") || 
                ! sink.at("componentId").is_number_integer() ||
                ! sink.contains("index") ||
                ! sink.at("index").is_number_integer()
            ){
                SPDLOG_ERROR("Cannot process AudioSink in with improper format: {}", sink.dump());
                success = false ;
                continue ;
            } 

            ConnectionRequest req ;
            req.inboundSocket = SocketType::SignalInbound ;
            req.outboundSocket = SocketType::SignalOutbound ;        
            req.outboundID = sink.at("componentId");
            req.outboundIdx = sink.at("index");
            req.inboundIdx = 0 ; // TODO: support multi-channel output
            
            const auto& connectionResponse = parseConnectionRequest(sock, req);
            if ( ! connectionResponse.contains("status") || connectionResponse.at("status") != "success" ){
                SPDLOG_ERROR("error requesting connection: {}", connectionResponse.dump());
                success = false ;
            }
        }
    }

    // connections to hardware midi
    if ( config.contains("rootMidiHandlers") && config.at("rootMidiHandlers").is_array() ){
        for ( const auto& id : config.at("rootMidiHandlers") ){
            if ( ! id.is_number_integer() ){
                SPDLOG_ERROR("Cannot process root midi handler with improper format: {}", id.dump() );
                success = false ;
                continue ;
            } 

            ConnectionRequest req ;
            req.inboundSocket = SocketType::MidiInbound ;
            req.outboundSocket = SocketType::MidiOutbound ;
            req.inboundID = id ;
            
            const auto& connectionResponse = parseConnectionRequest(sock, req);
            if ( ! connectionResponse.contains("status") || connectionResponse.at("status") != "success" ){
                SPDLOG_ERROR("error requestion connection: {}", connectionResponse.dump());
                success = false ;
            }
        }
    }

    // intercomponent connections
    if ( config.contains("components") && config.at("components").is_array() ){
        for ( const auto& component : config.at("components") ){
            if (
                ! component.is_object() ||
                ! component.contains("id") ||
                ! component.at("id").is_number_integer() ||
                ! component.contains("parameters") ||
                ! component.at("parameters").is_object() ||
                ! component.contains("type") ||
                ! component.at("type").is_number_integer()
            ){
                SPDLOG_ERROR("component is not in expected format: {}", component.dump()) ;
                success = false ;
                continue ;
            }

            // audio connections
            if ( component.contains("signalInputs") && component.at("signalInputs").is_array() ){
                for ( size_t inboundIdx = 0 ; inboundIdx < component.at("signalInputs").size(); ++inboundIdx ){
                    for ( const auto& outbound : component["signalInputs"][inboundIdx] ){
                        if ( 
                            ! outbound.contains("componentId") ||
                            ! outbound.at("componentId").is_number_integer() ||
                            ! outbound.contains("index") ||
                            ! outbound.at("index").is_number_integer()
                        ){
                            SPDLOG_ERROR("Cannot process component signal connection with improper format: {}", outbound.dump());
                            success = false ;
                            continue ;
                        } 

                        ConnectionRequest req ;
                        req.inboundID = component.at("id");
                        req.inboundIdx = inboundIdx ;
                        req.inboundSocket = SocketType::SignalInbound ;
                        req.outboundID = outbound.at("componentId");
                        req.outboundIdx = outbound.at("index");
                        req.outboundSocket = SocketType::SignalOutbound ;

                        const auto& connectionResponse = parseConnectionRequest(sock, req);
                        if ( ! connectionResponse.contains("status") || connectionResponse.at("status") != "success" ){
                            SPDLOG_ERROR("error requesting connection: {}", connectionResponse.dump());
                            success = false ;
                        }
                    }
                }
            }

            // midi connections
            if ( component.contains("midiListeners") && component.at("midiListeners").is_array() ){
                for ( const auto& inboundId : component.at("midiListeners") ){
                    if ( ! inboundId.is_number_integer() ){
                        SPDLOG_ERROR("Cannot process improperly formatted midi listener entry: {}", inboundId.dump() );
                        success = false ;
                        continue ;
                    } 
                    ConnectionRequest req ;
                    req.inboundID = inboundId ;
                    req.inboundSocket = SocketType::MidiInbound ;
                    req.outboundID = component.at("id") ;
                    req.outboundSocket = SocketType::MidiOutbound ;
                    
                    const auto& connectionResponse = parseConnectionRequest(sock, req);
                    if ( ! connectionResponse.contains("status") || connectionResponse.at("status") != "success" ){
                        SPDLOG_ERROR("error requesting connection: {}", connectionResponse.dump());
                        success = false ;
                    }
                }
            }

            // modulation connections
            for ( const auto& [p, data] : component.at("parameters").items() ){
                if ( ! data.contains("modulatorId") ) continue ;
                
                if ( ! data.at("modulatorId").is_number_integer() ){
                    SPDLOG_ERROR("Cannot process modulation connection with invalid modulatorId format: {}", data.at("modulatorId").dump());
                    success = false ;
                    continue ;
                }

                ParameterType param ;
                try {
                    param = parameterFromString(p);
                } catch (std::exception& e ){
                    SPDLOG_ERROR("Cannot process modulation connection with invalid parameter json: {}", p);
                    success = false ;
                    continue ;
                }
         
                ConnectionRequest req ;
                req.inboundID = component.at("id") ;
                req.inboundSocket = SocketType::ModulationInbound ;
                req.inboundParameter = param ;
                req.outboundID = data.at("modulatorId");
                req.outboundSocket = SocketType::ModulationOutbound ;
                
                const auto& connectionResponse = parseConnectionRequest(sock, req);
                if ( ! connectionResponse.contains("status") || connectionResponse.at("status") != "success" ){
                    SPDLOG_ERROR("error requesting connection: {}", connectionResponse.dump());
                    success = false ;
                }
            }
        }
    }

    return success ;

}

void ApiHandler::loadUpdateIds(json& j, const std::unordered_map<int, int>& idMap){
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
        std::vector<std::string> idArrayKeys = {"rootMidiHandlers", "midiListeners", "componentIds"};
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

const CollectionDescriptor& ApiHandler::getCollectionDescriptor(ComponentType t, CollectionType c) const {
    const ComponentDescriptor& descriptor = ComponentRegistry::getComponentDescriptor(t);
    int idx = descriptor.hasCollection(c);
    if ( idx == -1 ){
        std::string msg = fmt::format("Cannot retrieve collection {} from Component Type {}.", CollectionType::toString(c),  static_cast<char>(t) );
        SPDLOG_ERROR(msg);
        throw std::runtime_error(msg);
    }

    return descriptor.getCollection(idx);
}