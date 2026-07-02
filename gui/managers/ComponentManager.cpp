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

#include "managers/ComponentManager.hpp"
#include "meta/ComponentRegistry.hpp"
#include "api/ControlApiClient.hpp"
#include "api/DataApiClient.hpp"

ComponentManager::ComponentManager(QObject* parent):
    QObject(parent),
    models_(),
    parameters_(),
    modParameters_()
{
    connect(
        ControlApiClient::instance(), &ControlApiClient::dataReceived, 
        this, &ComponentManager::onControlMessageReceived
    );
    connect(
        DataApiClient::instance(), &DataApiClient::dataReceived,
        this, &ComponentManager::onDataMessageReceived
    );
}

ComponentManager::~ComponentManager(){
    for ( const auto& m : models_ ) m.second->deleteLater();
    for ( const auto& p : parameters_ ) p.second->deleteLater();
    for ( const auto& mp: modParameters_ ) mp.second->deleteLater();
}

void ComponentManager::requestAddComponent(ComponentType type){
    json j ;
    auto descriptor = ComponentRegistry::getComponentDescriptor(type);

    j["action"] = "add_component" ;
    j["name"] = descriptor.name ;
    ControlApiClient::instance()->sendMessage(j); 
}

void ComponentManager::requestRemoveComponent(int componentId){    
    json j ;
    j["action"] = "remove_component" ;
    j["componentId"] = componentId ;
    ControlApiClient::instance()->sendMessage(j);
}

void ComponentManager::requestParameterUpdate(int componentId, ParameterType p, ParameterValue v){
    json j ;
    j["action"] = "set_parameter" ;
    j["componentId"] = componentId ;
    j["parameter"] = GET_PARAMETER_TRAIT_MEMBER(p,name);
    j["value"] = ParameterValueToJson(v) ;
    
    ControlApiClient::instance()->sendMessage(j); 
}

void ComponentManager::requestCollectionUpdate(CollectionRequest req){
    json obj = req ;
    ControlApiClient::instance()->sendMessage(obj); 
}

void ComponentManager::requestModulationDepthUpdate(int componentId, ParameterType p, double depth){
    json obj ;
    obj["action"] = "set_modulation_depth" ;
    obj["componentId"] = componentId ;
    obj["parameter"] = GET_PARAMETER_TRAIT_MEMBER(p, name);
    obj["depth"] = depth ;

    ControlApiClient::instance()->sendMessage(obj);
}

void ComponentManager::requestModulationStrategyUpdate(int componentId, ParameterType p, ModulationStrategy strategy){
    json obj ;
    obj["action"] = "set_modulation_strategy" ;
    obj["componentId"] = componentId ;
    obj["parameter"] = GET_PARAMETER_TRAIT_MEMBER(p, name);
    obj["strategy"] = static_cast<int>(strategy) ;

    ControlApiClient::instance()->sendMessage(obj);
}

void ComponentManager::requestModelSync(int componentId){
    json obj ;
    obj["action"] = "sync_component" ;
    obj["componentId"] = componentId ;
    
    ControlApiClient::instance()->sendMessage(obj);
}

void ComponentManager::requestSetFile(int componentId, std::string path){
    json obj ;
    obj["action"] = "set_file_path" ;
    obj["componentId"] = componentId ;
    obj["path"] = path ;

    ControlApiClient::instance()->sendMessage(obj);
}

ComponentModel* ComponentManager::getModel(int componentId) const {
    if ( !models_.contains(componentId) ){
        SPDLOG_WARN("No component model found with id = {}",componentId);
        return nullptr ;
    } 

    return models_.at(componentId) ;
}

ComponentParameters* ComponentManager::getParameters(int componentId) const {
    if ( !parameters_.contains(componentId) ){
        return nullptr ;
    } 

    return parameters_.at(componentId) ;
}

ModulationParameters* ComponentManager::getModulationParameters(int componentId) const {
    if ( !modParameters_.contains(componentId) ){
        return nullptr ;
    } 

    return modParameters_.at(componentId);
}

void ComponentManager::addComponent(int componentId, ComponentType type){
    auto model = new ComponentModel(componentId, type);
    models_[componentId] = model ;

    connect(
        model, &ComponentModel::requestBufferData,
        this, &ComponentManager::onRequestBufferData
    );
    
    ComponentParameters* params = nullptr ;
    if ( model->getDescriptor().shouldShowControlParameters() ){
        params = new ComponentParameters(model);
        parameters_[componentId] = params ;
        connect(
            params, &ComponentParameters::parameterEdited,
            this, &ComponentManager::onParameterEdited
        );
        connect(
            params, &ComponentParameters::fileSelected,
            this, &ComponentManager::onFileSelected
        );
    }

    
    ModulationParameters* modParams = nullptr ;
    if ( model->getDescriptor().shouldShowModulationParameters() ){
        modParams = new ModulationParameters(model);
        modParameters_[componentId] = modParams ;
        connect(
            modParams, &ModulationParameters::modulationDepthEdited,
            this, &ComponentManager::onModulationDepthEdited
        );
        connect(
            modParams, &ModulationParameters::modulationStrategyEdited,
            this, &ComponentManager::onModulationStrategyEdited
        );
    }

    // handle collection widget if exists
    auto cw = getCollectionWidget(params);
    if ( cw ){
        connect(
            cw, &CollectionWidget::collectionEdited,
            this, &ComponentManager::onCollectionEdited
        );
    }

    emit componentAdded(componentId, type);
}

void ComponentManager::removeComponent(int componentId){
    auto it = models_.find(componentId);

    if ( it == models_.end() ){
        SPDLOG_WARN("could not find model for component to delete");
        return ;
    }

    if ( models_.contains(componentId) && models_.at(componentId) ){
        models_.at(componentId)->deleteLater();
        models_.erase(componentId);
    }
    
    if ( parameters_.contains(componentId) && parameters_.at(componentId) ){
        parameters_.at(componentId)->deleteLater();
        parameters_.erase(componentId);
    }
    
    if ( modParameters_.contains(componentId) && modParameters_.at(componentId) ){
        modParameters_.at(componentId)->deleteLater();
        modParameters_.erase(componentId);
    }

    emit componentRemoved(componentId);
}

void ComponentManager::setParameterValue(int componentId, ParameterType p, const json& value){
        // dispatch to set parameter value with correct variant
        ParameterValue v ; 
        switch(p){
        #define X(name) \
        case ParameterType::name: \
            v = static_cast<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>(value); \
            break ; 
        PARAMETER_TYPE_LIST
        #undef X     
        default:
            break ;
        }

        getModel(componentId)->setParameterValue(p,v);
        return ;
}

void ComponentManager::syncModel(const json& msg){
    int id = msg.at("componentId");
    const json& data = msg.at("data");

    auto model = getModel(id);
    if ( !model ){
        SPDLOG_WARN("Cannot find model with id = {}", id);
        return ;
    }

    if ( data.contains("parameters") ){
        for ( const auto& [p, obj] : data.at("parameters").items() ){
            if ( ! obj.contains("currentValue") ) continue ;
            setParameterValue(id, stringToParameter(p), obj.at("currentValue") );
        }
    }
}

void ComponentManager::setFile(int componentId, std::string path){
    auto model = getModel(componentId);
    if ( !model ){
        SPDLOG_WARN("Cannot find model with id = {}", componentId);
        return ;
    }

    model->setFile(path);
}

CollectionWidget* ComponentManager::getCollectionWidget(ComponentParameters* params) const {
    if ( !params ) return nullptr ;
    auto detailed = params->getDetailedEditor();
    if ( !detailed ) return nullptr ;

    CollectionWidget* cw = dynamic_cast<CollectionWidget*>(detailed);
    if ( !cw ){
        cw = detailed->findChild<CollectionWidget*>();
    }
    
    return cw ;
}

bool ComponentManager::handleCollectionApiResponse(const json& msg){
    // note: there is no centralized model for managing Collections. So we will 
    // instead break the pattern here and only do this for specialized widget stored
    // in the component editor. 

    // bool response is to determine if the event should be considered "handled" so
    // that we don't eat up the wrong types of request
    CollectionRequest req ;
    try {
        req = msg ;
    } catch (std::exception& e){
        return false ;
    }

    auto it = parameters_.find(req.componentId);
    if ( it == parameters_.end() ){
        SPDLOG_WARN("Could not find model with componentId {}. Will not process collection request", 
            req.componentId);
        return true ;
    }
    
    auto cw = getCollectionWidget(it->second);
    if ( cw ){
        cw->updateCollection(req);
    }
    
    return true ;
}


void ComponentManager::onControlMessageReceived(const json& msg){
    QString action = QString::fromStdString(msg["action"]);
    bool success = msg.at("status") == "success" ;

    if ( action == "add_component" && success ){
        int id = msg.at("componentId");
        ComponentType type = ComponentRegistry::getComponentDescriptor(
            msg.at("name").get<std::string>()
        ).type ;
        addComponent(id, type);
        return ;
    }

    if ( action == "remove_component" && success ){
        int id = msg.at("componentId") ;
        removeComponent(id);
        return ;
    }

    if ( action == "sync_component" && success ){
        syncModel(msg);
        return ;
    }

    if ( action == "set_parameter" && success ){
        int id = msg.at("componentId");
        auto it = models_.find(id);
        if ( it == models_.end() ){
            SPDLOG_WARN("Could not find model with componentId {}. Will not process set parameter request", 
                id);
            return ;
        }

        ParameterType p = stringToParameter(msg.at("parameter"));

        setParameterValue(id, p, msg.at("value"));
        return ;
    }

    if ( action == "set_modulation_depth" && success ){
        int id = msg["componentId"];
        auto it = models_.find(id);
        if ( it == models_.end() ){
            SPDLOG_WARN("Could not find model with componentId {}. Will not process modulation depth request",
                id); 
            return ;
        }

        ParameterType p = stringToParameter(msg.at("parameter"));
        ModulationModel* m = it->second->getModulationModel(p);
        if ( !m ){
            SPDLOG_WARN("Could not find modulation model for parameter {} from componentId {}.",
                GET_PARAMETER_TRAIT_MEMBER(p, name), it->second->getId());
            return ;
        }

        m->setDepth(msg.at("depth"));
        return ;
    }

    if ( action == "set_modulation_strategy" && success ){
        int id = msg.at("componentId");
        auto model = getModel(id);
        if (  !model ){
            SPDLOG_WARN("Could not find model with componentId {}. Will not process modulation strategy request",
                id);
            return ;
        }

        ParameterType p = stringToParameter(msg.at("parameter"));
        ModulationModel* m = model->getModulationModel(p);
        if ( !m ){
            SPDLOG_WARN("Could not find modulation model for parameter {} from componentId {}.",
                GET_PARAMETER_TRAIT_MEMBER(p, name), id);
            return ;
        }

        m->setStrategy(static_cast<ModulationStrategy>(msg.at("strategy")));
        return ;
    }

    for ( const auto& [_,actionStr] : CollectionRequest::actionMap ){
        if ( action == actionStr && success ){
             handleCollectionApiResponse(msg);
        }
    }
}

void ComponentManager::onDataMessageReceived(DataDescriptor header, std::vector<double> buffer){
    auto* model = getModel(header.componentId);
    if ( !model ){
        SPDLOG_WARN("no model with component id {} is available.", header.componentId); 
        return ;
    }

    model->setBuffer(header.channel, buffer);
}

void ComponentManager::onParameterEdited(int componentId, ParameterType p, ParameterValue value){
    requestParameterUpdate(componentId, p, value);
}

void ComponentManager::onCollectionEdited(CollectionRequest req ){
    requestCollectionUpdate(req);
}

void ComponentManager::onModulationDepthEdited(int componentId, ParameterType p, double depth){
    requestModulationDepthUpdate(componentId, p, depth);
}

void ComponentManager::onModulationStrategyEdited(int componentId, ParameterType p, ModulationStrategy strategy){
    requestModulationStrategyUpdate(componentId, p, strategy);
}

void ComponentManager::onFileSelected(int componentId, std::string path){
    requestSetFile(componentId, path);
}

void ComponentManager::onRequestBufferData(int componentId, size_t channel){
    json obj ;
    obj["action"] = "get_buffer_data" ;
    obj["componentId"] = componentId ;
    obj["channel"] = channel ;

    ControlApiClient::instance()->sendMessage(obj);
}

void ComponentManager::onConnectionAdded(const ConnectionRequest& req){
    // handle modulation connection tracking
    if ( 
        req.inboundSocket == SocketType::ModulationInbound && 
        req.inboundID.has_value() &&
        req.inboundParameter.has_value()
    ){
        auto modParams = getModulationParameters(req.inboundID.value());
        if ( !modParams ) return ;
        modParams->setConnectionStatus(req.inboundParameter.value(), true);
    }

    // handle upstream buffer tracking
    if (
        req.inboundSocket == SocketType::BufferInbound &&
        req.inboundID.has_value() &&
        req.inboundIdx.has_value()
    ){
        ComponentModel* downstream = getModel(req.inboundID.value());
        ComponentModel* upstream = getModel(req.outboundID.value());

        if ( !upstream || !downstream || !upstream->hasBuffer(req.outboundIdx.value()) ) return ;
        downstream->setUpstreamModel(
            req.inboundIdx.value(),
            req.outboundIdx.value(),
            upstream
        );
    }
}

void ComponentManager::onConnectionRemoved(const ConnectionRequest& req){
    // handle modulation connection tracking
    if ( 
        req.inboundSocket == SocketType::ModulationInbound && 
        req.inboundID.has_value() &&
        req.inboundParameter.has_value()
    ){
        auto modParams = getModulationParameters(req.inboundID.value());
        if ( !modParams ) return ;
        modParams->setConnectionStatus(req.inboundParameter.value(), false);
    }

    // handle upstream buffer tracking
    if (
        req.inboundSocket == SocketType::BufferInbound &&
        req.inboundID.has_value() &&
        req.inboundIdx.has_value()
    ){
        ComponentModel* downstream = getModel(req.outboundID.value());

        if ( !downstream ) return ;
        downstream->clearUpstreamModel(req.inboundIdx.value());
    }
}