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
#include "midi/MidiControlRouter.hpp"

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
    handlers_["midi_learn"] = [this](const json& request){ return midiLearn(request); };
    handlers_["get_midi_control"] = [this](const json& request){ return getMidiControl(request); };
    handlers_["set_midi_control"] = [this](const json& request){ return setMidiControl(request); };
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
    if ( !response.contains("status") ){
        if ( err == "" ){
            response["status"] = "success" ;
        } else {
            response["status"] = "failed" ;
            response["error"] = err ;
            SPDLOG_ERROR("Api Request Failed: {}", err);
        }    
    }

    std::string r = response.dump() + '\n' ;
    SPDLOG_INFO("sending API response: {}", r.c_str());
    for ( const auto& sock : clientSockets_ ){
        send(sock, r.c_str(), r.size(), 0);
    }
    return response ;
}


void ControlApiHandler::handleClientMessage(const std::string& jsonStr){
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
    
    auto handler = handlers_.find(action);
    if ( handler == handlers_.end() ){
        sendApiResponse(request, "unknown action requested: " + action );
        return ;
    }
    
    json response ;
    try {
        response = handler->second(request);
    } catch (const std::exception& e){
        sendApiResponse(request, std::string(e.what()));
        return ;
    }

    sendApiResponse(response);
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
    return response ;
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
    return response ;
}

json ControlApiHandler::setAudioDevice(const json& request){
    json response = request ;
    int deviceId ;
    std::string err ;
    
    deviceId = response.at("device_id");
    
    if ( engine_->setAudioDeviceId(deviceId) ){
        return response ;
    } else {
        throw std::runtime_error("failed to set audio device");
    }
}

json ControlApiHandler::getAudioConfig(const json& request){
    json response = request ;

    response["device_id"] = engine_->getAudioDeviceId();
    response["output_channels"] = engine_->signalController.getNumChannels();

    return response ;
}

json ControlApiHandler::setMidiDevice(const json& request){
    json response = request ;
    int deviceId ;
    
    deviceId = response.at("device_id");

    if ( engine_->setMidiDeviceId(deviceId) ){
        return response ;
    } else {
        throw std::runtime_error("failed to set MIDI device");
    }
}

json ControlApiHandler::setState(const json& request){
    json response = request ;
    std::string state ;

    state = response.at("state");
    
    if ( state == "run" ){
        engine_->run();
        return response ;
    }

    if ( state == "stop" ){
        engine_->stop();
        return response ;
    }

    throw std::runtime_error("Unrecognized engine state requested: " + state);
}

json ControlApiHandler::getConfiguration(const json& request){
    json response = request ;
    response["data"] = engine_->serialize();
    return response ;
}

json ControlApiHandler::loadPatch(const json& request){
    json response = request ;
    
    // create components
    if ( 
        !response.contains("components") ||
        !response.at("components").is_array() 
    ){
        throw std::runtime_error("components array not properly defined");
    }

    std::unordered_map<int,int> idMap ; // old id, new id
    if ( ! loadCreateComponent(response["components"], idMap) ){
        throw std::runtime_error("Error creating components when loading patch");
    }

    loadUpdateIds(response, idMap);

    // connect components
    if ( response.contains("connections") ){
        if ( !loadConnectComponent(response.at("connections")) ){
            throw std::runtime_error("Error connecting components when loading patch");
        }
    }

    // reconnect midi controls
    if ( response.contains("midi_controls") ){
        if ( ! loadMidiControls(response.at("midi_controls")) ){
            throw std::runtime_error("Error configuring midi controls when loading patch");
        }
    }
    
    return response ;
}

json ControlApiHandler::addComponent(const json& request){
    json response = request ;
    ComponentType type ;
    std::string name ;

    name = response.at("name");
    type = ComponentRegistry::getComponentDescriptor(name).type ;

    ComponentId id = engine_->componentFactory.createFromJson(type, name, getDefaultConfig(type));
    response["componentId"] = id ;

    return response ;
}

json ControlApiHandler::removeComponent(const json& request){
    json response = request ;
    ComponentId id = request.at("componentId");

    id = response.at("componentId");
    
    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error("component not found.");
    }

    // query each subsystem and remove connections, if exist
    bool allRemoved = true ;
    auto connections = engine_->getComponentConnections(id);
    for ( auto c : connections ){
        c.remove = true ;
        json j = c ;
        auto cresponse = parseConnectionRequest(j);
        sendApiResponse(cresponse);
        allRemoved = allRemoved && cresponse.contains("status") && cresponse.at("status") == "success" ;
    }

    if ( !allRemoved ){
        throw std::runtime_error("at least one component connection could not be removed.");
    }

    engine_->componentManager.remove(id);
    return response ;    
}

json ControlApiHandler::syncComponent(const json& request){
    json response = request ;

    int componentId = response.at("componentId");
    
    auto c = engine_->componentManager.getRaw(componentId);
    if ( !c ){
        throw std::runtime_error(fmt::format("could not find component with id {}", componentId));
    }

    response["data"] = engine_->componentManager.serializeComponent(c);
    return response ;
}

json ControlApiHandler::parseConnectionRequest(const json& request){
    json response = request ;
    ConnectionRequest req ;

    try {
        req = response.get<ConnectionRequest>() ;
    } catch (const std::exception& e){
        throw std::runtime_error("could not parse connection request");
    }

    if ( ! req.valid() ){
        throw std::runtime_error("parsed connection request is invalid");
    }

    if ( routeConnectionRequest(req)){
        return response ;
    } else {
        throw std::runtime_error("failed to handle requested connection event");
    }
}

json ControlApiHandler::getParameter(const json& request){
    json response = request ;
    ComponentId id = response.at("componentId");
    ParameterType param = stringToParameter(response.at("parameter"));

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id = {} not found for parameter request",
            id
        ));
    }

    response["value"] = c->getParameters()->getValueDispatch(param);
    return response ;
}

json ControlApiHandler::setParameter(const json& request){
    json response = request ;
    ComponentId id = response.at("componentId");
    ParameterType param = stringToParameter(response.at("parameter"));

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id = {} not found for parameter request",
            id
        ));
    }

    bool setSuccess = c->getParameters()->setValueDispatch(param, response.at("value"));
     // feed the value back to the client (due to limiting or other behaviors)
    response["value"] = c->getParameters()->getValueDispatch(param);
    if ( setSuccess ){
        return response ;
    } else {
        throw std::runtime_error("could not set component parameter.");
    }
}

json ControlApiHandler::getParameterDefault(const json& request){
    json response = request ;
    ComponentId id = response.at("componentId");
    ParameterType param = stringToParameter(response.at("parameter"));
    
    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id = {} not found for parameter request",
            id
        ));
    }

    response["value"] = c->getParameters()->getDefaultDispatch(param);
    return response ;
}

json ControlApiHandler::setParameterDefault(const json& request){
    json response = request ;
    ComponentId id = response.at("componentId");
    ParameterType param = stringToParameter(response.at("parameter"));

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id = {} not found for parameter request",
            id
        ));
    }

    if ( c->getParameters()->setDefaultDispatch(param, response["value"]) ){
        return response ;
    } else {
        throw std::runtime_error("Could not set component parameter default value.");
    }
}

json ControlApiHandler::getParameterValueRange(const json& request){
    json response = request ;
    ComponentId id = response.at("componentId");
    ParameterType param = stringToParameter(response.at("parameter"));

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id = {} not found for parameter request",
            id
        ));
    }

    response["minimum"] = c->getParameters()->getMinDispatch(param);
    response["maximum"] = c->getParameters()->getMaxDispatch(param);

    return response ;
}

json ControlApiHandler::setParameterValueRange(const json& request){
    json response = request ;
    ComponentId id = response.at("componentId");
    ParameterType param = stringToParameter(response.at("parameter"));

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id = {} not found for parameter request",
            id
        ));
    }

    if ( !c->getParameters()->setMinDispatch(param, response["minimum"]) ){
        throw std::runtime_error("Could not set component parameter minimum value.");
    }

    if ( !c->getParameters()->setMaxDispatch(param, response["maximum"]) ){
        throw std::runtime_error("Could not set component parameter maximum value.");
    }

    return response ;
}

json ControlApiHandler::resetParameter(const json& request){
    json response = request ;
    ComponentId id = request.at("componentId");
    ParameterType param = stringToParameter(response.at("parameter"));

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id = {} not found for parameter request",
            id
        ));
    }

    c->getParameters()->getParameter(param)->resetValue();
    return response ;
}

json ControlApiHandler::parseCollectionRequest(const json& request){
    json response = request ;
    ComponentId id = request.at("componentId");

    auto c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id = {} not found for parameter request",
            id
        ));
    }

    const CollectionDescriptor& cd = getCollectionDescriptor(c->getType());
    CollectionRequest req ;
    try {
        req = response ;
    } catch (const std::exception& e){
        throw std::runtime_error(fmt::format(
            "could not parse collection request: {}",
            e.what()
        ));
    }

    if ( !cd.isValid() ){
        throw std::runtime_error(fmt::format(
            "collection descriptor for component type {} is malformed.",
            ComponentRegistry::getComponentDescriptor(c->getType()).name
        ));
    }

    if ( !req.valid(cd) ){
        throw std::runtime_error(fmt::format(
            "The received collection request for component {} is invalid for the specified collection structure",
            ComponentRegistry::getComponentDescriptor(c->getType()).name
        ));
    }

    switch(req.action){
    case CollectionAction::ADD:       return addCollectionValue(c, cd, req);
    case CollectionAction::REMOVE:    return removeCollectionValue(c, cd, req);
    case CollectionAction::GET:       return getCollectionValue(c, cd, req);
    case CollectionAction::GET_ALL:   return getCollectionValues(c, cd, req);
    case CollectionAction::SET:       return setCollectionValue(c, cd, req);
    case CollectionAction::RESET:     return resetCollection(c, cd, req);
    default:                          
        throw std::runtime_error("Unexpected Collection Action received");
    }
}

json ControlApiHandler::addCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request){
    ParameterMap* params = c->getParameters();
    try {
        json response = request ;

        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            request.index = params->addCollectionValueDispatch(cd.params[0], request.value);
            params->getCollection(cd.params[0])->notifyListeners();
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                size_t idx = c->getParameters()->addCollectionValueDispatch(cd.params[0], (*request.value)[i]);
                if ( i == 0 ) request.index = idx ;
            }
            params->getCollection(cd.params[0])->notifyListeners();
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                request.index = c->getParameters()->addCollectionValueDispatch(
                    cd.params[i], 
                    (*request.value)[GET_PARAMETER_TRAIT_MEMBER(cd.params[i], name)]
                );
                params->getCollection(cd.params[i])->notifyListeners();
            }
            break ;
        default: 
            throw std::runtime_error("unrecognized collection structure");
        }
    } catch (const std::exception& e){
        throw std::runtime_error(fmt::format(
            "failed to add collection value: {}",
            e.what()
        ));
    }

    return request ;
}

json ControlApiHandler::removeCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request){
    ParameterMap* params = c->getParameters();
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            params->removeCollectionValueDispatch(cd.params[0], *request.index);
            params->getCollection(cd.params[0])->notifyListeners();
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                params->removeCollectionValueDispatch(cd.params[0], *request.index);
            }
            params->getCollection(cd.params[0])->notifyListeners();
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                params->removeCollectionValueDispatch(cd.params[i], *request.index);
                params->getCollection(cd.params[i])->notifyListeners();
            }
            break ;
        default: 
            throw std::runtime_error("unrecognized collection structure");
        }
    } catch (const std::exception& e){
        throw std::runtime_error(fmt::format(
            "failed to remove collection value: {}",
            e.what()
        ));
    }

    return request ;
}

json ControlApiHandler::getCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request){
    ParameterMap* params = c->getParameters();
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            request.value = params->getCollectionValueDispatch(cd.params[0], *request.index);
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                (*request.value)[i] = params->getCollectionValueDispatch(cd.params[0], *request.index);
            }
            break ;
        case CollectionStructure::SYNCHRONIZED:
            request.value = json::object();
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                (*request.value)[GET_PARAMETER_TRAIT_MEMBER(cd.params[i], name)] = 
                    params->getCollectionValueDispatch(cd.params[i], *request.index);
            }
            break ;
        default: 
            throw std::runtime_error("unrecognized collection structure");
        }
    } catch (const std::exception& e){
        throw std::runtime_error(fmt::format(
            "failed to get collection value: {}",
            e.what()
        ));
    }

    return request ;
}

json ControlApiHandler::getCollectionValues(BaseComponent* c, const CollectionDescriptor& cd, CollectionRequest& request){
    ParameterMap* params = c->getParameters();
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
        case CollectionStructure::GROUPED:
            request.value = params->getCollectionValuesDispatch(cd.params[0]);
            break ;
        case CollectionStructure::SYNCHRONIZED:
            request.value = json::object();
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                (*request.value)[GET_PARAMETER_TRAIT_MEMBER(cd.params[i], name)] = 
                    params->getCollectionValuesDispatch(cd.params[i]);
            }
            break ;
        default: 
            throw std::runtime_error("unrecognized collection structure");
        }
    } catch (const std::exception& e){
        throw std::runtime_error(fmt::format(
            "failed to get collection values: {}",
            e.what()
        ));
    }

    return request ;
}


json ControlApiHandler::setCollectionValue(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request){
    ParameterMap* params = c->getParameters();
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            params->setCollectionValueDispatch(cd.params[0], *request.index, *request.value);
            params->getCollection(cd.params[0])->notifyListeners();
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                params->setCollectionValueDispatch(cd.params[0], *request.index + i, (*request.value)[i]);
            }
            params->getCollection(cd.params[0])->notifyListeners();
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                params->setCollectionValueDispatch(cd.params[i], *request.index, 
                    (*request.value)[GET_PARAMETER_TRAIT_MEMBER(cd.params[i], name)]);
                params->getCollection(cd.params[i])->notifyListeners();
            }
            break ;
        default: 
            throw std::runtime_error("unrecognized collection structure");
        }
    } catch (const std::exception& e){
        throw std::runtime_error(fmt::format(
            "failed to set collection value: {}",
            e.what()
        ));
    }

    json response = request ;
    return response ;
}

json ControlApiHandler::resetCollection(BaseComponent* c, const CollectionDescriptor& cd, const CollectionRequest& request){
    ParameterMap* params = c->getParameters();
    try {
        switch ( cd.structure ){
        case CollectionStructure::INDEPENDENT:
            params->resetCollectionDispatch(cd.params[0]);
            params->getCollection(cd.params[0])->notifyListeners();
            break ;
        case CollectionStructure::GROUPED:
            for ( size_t i = 0 ; i < cd.groupSize ; ++i ){
                params->resetCollectionDispatch(cd.params[0]);
            }
            params->getCollection(cd.params[0])->notifyListeners();
            break ;
        case CollectionStructure::SYNCHRONIZED:
            for ( size_t i = 0 ; i < cd.params.size() ; ++i ){
                params->resetCollectionDispatch(cd.params[i]);
                params->getCollection(cd.params[i])->notifyListeners();
            }
            break ;
        default: 
            throw std::runtime_error("unrecognized collection structure");
        }
    } catch (const std::exception& e){
        throw std::runtime_error(fmt::format(
            "failed to set collection value: {}",
            e.what()
        ));
    }

    return request ;
}

json ControlApiHandler::getModulationStrategy(const json& request){
    ComponentId id = request.at("componentId");
    ParameterType p = stringToParameter(request.at("parameter"));
    json response = request ;

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        throw std::runtime_error(fmt::format(
            "Could not find component with id = {}",
            id
        ));
    }

    auto descriptor = ComponentRegistry::getComponentDescriptor(component->getType());
    const auto& modulatable = descriptor.modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        throw std::runtime_error(fmt::format(
            "Parameter {} is not modulatable for component named {}.",
            std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)),
            descriptor.name
        ));
    }
    
    response["strategy"] = component->getParameterModulationStrategy(p);
    return response ;
}

json ControlApiHandler::setModulationStrategy(const json& request){
    ComponentId id = request.at("componentId");
    ParameterType p = stringToParameter(request.at("parameter"));
    ModulationStrategy s = static_cast<ModulationStrategy>(request.at("strategy"));
    json response = request ;

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        throw std::runtime_error(fmt::format(
            "Could not find component with id = {}",
            id
        ));
    }

    auto descriptor = ComponentRegistry::getComponentDescriptor(component->getType());
    const auto& modulatable = descriptor.modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        throw std::runtime_error(fmt::format(
            "Parameter {} is not modulatable for component named {}.",
            std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)),
            descriptor.name
        ));
    }

    component->setParameterModulationStrategy(p, s);
    return response ;
}

json ControlApiHandler::getModulationDepth(const json& request){
    ComponentId id = request.at("componentId");
    ParameterType p = stringToParameter(request.at("parameter"));

    auto component = engine_->componentManager.getRaw(id);
    if ( !component ){
        throw std::runtime_error(fmt::format(
            "Could not find component with id = {}",
            id
        ));
    }
    
    auto descriptor = ComponentRegistry::getComponentDescriptor(component->getType());
    const auto& modulatable = descriptor.modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        throw std::runtime_error(fmt::format(
            "Parameter {} is not modulatable for component named {}.",
            std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)),
            descriptor.name
        ));
    }

    json response = request ;
    response["depth"] = component->getParameterDepth(p);
    return response ;
}


json ControlApiHandler::setModulationDepth(const json& request){
    ComponentId id = request.at("componentId");
    ParameterType p = stringToParameter(request.at("parameter"));
    double depth = request.at("depth");
    json response = request ;

    auto component = engine_->componentManager.getRaw(id);
    if (!component){
        throw std::runtime_error(fmt::format("Could not find component with id = {}", id));
    }
    
    auto descriptor = ComponentRegistry::getComponentDescriptor(component->getType());
    const auto& modulatable = descriptor.modulatableParameters ;
    auto it = std::find(modulatable.begin(), modulatable.end(), p);
    if ( it == modulatable.end() ){
        throw std::runtime_error(fmt::format(
            "Parameter {} is not modulatable for component named {}.",
            std::string(GET_PARAMETER_TRAIT_MEMBER(p, name)),
            descriptor.name
        ));
    }

    component->setParameterDepth(p, depth);
    return response ;
}

json ControlApiHandler::getFilePath(const json& request){
    json response = request ;
    ComponentId id = response.at("componentId");

    FileComponent* c = engine_->componentManager.getFileComponent(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "File component with id {} not found",
            id
        ));
    }

    response["path"] = c->getPath();

    return response ;
}

json ControlApiHandler::setFilePath(const json& request){
    json response = request ;
    ComponentId id = response.at("componentId");
    std::string path =  response.at("path");

    FileComponent* c = engine_->componentManager.getFileComponent(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "File component with id {} not found",
            id
        ));
    }

    c->setPath(path);
    return response ;
}

json ControlApiHandler::getBufferData(const json& request){
    json response = request ;
    ComponentId id = request.at("componentId");
    size_t channel = response.at("channel");


    AudioBufferComponent* c = engine_->componentManager.getBufferComponent(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Buffer component with id {} not found",
            id
        ));
    }

    if ( channel >= c->getNumOutputs() ){
        throw std::runtime_error(fmt::format(
            "Invalid channel number for buffer component. Got {}, but only has {} channels",
            channel, c->getNumOutputs()
        ));
    }

    const auto& buffer = c->getBuffer(channel);
    DataDescriptor header {
        .componentId = static_cast<uint32_t>(id),
        .channel = static_cast<uint32_t>(channel),
        .size = static_cast<uint64_t>(buffer.size() * sizeof(double))
    };

    DataApiHandler::instance()->sendApiData(header, c->getBuffer(channel));
    return response ;

}

json ControlApiHandler::midiLearn(const json& request){
    // this is a special case. if we don't have a value/control type,
    // then we need to tell the control router to start a learn session
    // otherwise, we just pass through the response from the control router
    // which calls the api directly
    if ( request.contains("value") && request.contains("control_type") ){
        return request ;    
    }

    MidiControlRouter::instance()->setLearning(true);
    json response = request ;
    response["status"] = "pending" ;
    return response ;
}

json ControlApiHandler::getMidiControl(const json& request){
    ComponentId id = request.at("componentId");
    ParameterType p = stringToParameter(request.at("parameter"));

    BaseComponent* c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id {} not found",
            id
        ));
    }

    uint8_t ctrl = MidiControlRouter::instance()->getControlForRoute(c, p);

    if ( ctrl == MidiControlRouter::INVALID_ROUTE ){
        throw std::runtime_error(fmt::format(
            "no midi control route specified for component with"
            "id {} on {}",
            id, GET_PARAMETER_TRAIT_MEMBER(p, name)
        ));
    }

    json response = request ;
    response["value"] = ctrl ;

    return response ;
}

json ControlApiHandler::setMidiControl(const json& request){
    ComponentId id = request.at("componentId");
    ParameterType p = stringToParameter(request.at("parameter"));
    uint8_t ctrl = request.at("value");

    BaseComponent* c = engine_->componentManager.getRaw(id);
    if ( !c ){
        throw std::runtime_error(fmt::format(
            "Component with id {} not found",
            id
        ));
    }

    if ( ctrl > 127 ){
        throw std::runtime_error(fmt::format(
            "midi control identifier {} is not valid",
            ctrl
        ));
    }

    auto router = MidiControlRouter::instance();
    if ( request.contains("control_type") ){
        MidiControlRouter::ControlType ctrlType ;
        try {
             ctrlType = MidiControlRouter::stringToControlType(request.at("control_type"));
        } catch ( std::exception& e){
            throw std::runtime_error(fmt::format(
                "cannot process set midi request, control type is invalid: {}", 
                request.at("control_type").dump()
            ));
        }
        router->setMidiControlType(ctrl, ctrlType);
    }

    
    router->registerRoute(ctrl, c, p);

    return request ;
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
            sendApiResponse(componentResponse);
            idMap[id] = componentResponse["componentId"];

            for ( const auto& [p, data] : params.items() ){
                if ( ! data.contains("currentValue") ){
                    continue ; // just has modulation
                }
                parameterRequest["action"] = "set_component_parameter" ;
                parameterRequest["componentId"] = idMap[id] ;
                parameterRequest["parameter"] = p ;
                parameterRequest["value"] = data.at("currentValue") ;
                auto parameterResponse = setParameter(parameterRequest);
                sendApiResponse(parameterResponse);
            }
        } catch ( const std::exception& e ){
            SPDLOG_WARN("Error creating component: {}", std::string(e.what()));
            success = false ;
        }
    }

    return success ;
}

bool ControlApiHandler::loadConnectComponent(const json& connections){
    if ( !connections.is_array() ) return false ;

    bool success = true ;
    for ( auto c : connections ){
        ConnectionRequest request ;
        try {
            request = c ;
        } catch (std::exception& e){
            SPDLOG_ERROR("Could not parse {} to ConnectionRequest.", c.dump());
            success = false ;
        }

        auto connectionResponse = parseConnectionRequest(request);
        sendApiResponse(connectionResponse);
        if ( ! connectionResponse.contains("status") || connectionResponse.at("status") != "success" ){
            SPDLOG_ERROR("error requesting connection: {}", connectionResponse.dump());
            success = false ;
        }
    }

    return success ;

}

bool ControlApiHandler::loadMidiControls(const json& controls){
    if ( !controls.is_array() ) return false ;

    bool success = true ;
    for ( auto c : controls ){
        try {
            auto response = setMidiControl(c);
            sendApiResponse(response);
        } catch (std::exception& e){
            SPDLOG_ERROR("error loading midi control: {}", e.what());
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

const CollectionDescriptor& ControlApiHandler::getCollectionDescriptor(ComponentType t) const {
    const ComponentDescriptor& descriptor = ComponentRegistry::getComponentDescriptor(t);
    int idx = descriptor.hasCollection();
    if ( idx == -1 ){
        std::string msg = fmt::format(
            "Cannot retrieve collection from Component Type {}.",  
            ComponentRegistry::getComponentDescriptor(t).name
        );
        SPDLOG_ERROR(msg);
        throw std::runtime_error(msg);
    }

    return descriptor.getCollection();
}