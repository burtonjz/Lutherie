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
#include "api/ApiClient.hpp"

ComponentManager::ComponentManager(QObject* parent):
    QObject(parent),
    models_(),
    parameters_(),
    modParameters_()
{
    connect(
        ApiClient::instance(), &ApiClient::dataReceived, 
        this, &ComponentManager::onApiDataReceived
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
    j["type"] = static_cast<int>(type);
    ApiClient::instance()->sendMessage(j); 
}

void ComponentManager::requestRemoveComponent(int componentId){    
    json j ;
    j["action"] = "remove_component" ;
    j["componentId"] = componentId ;
    ApiClient::instance()->sendMessage(j);
}

void ComponentManager::requestParameterUpdate(int componentId, ParameterType p, ParameterValue v){
    json j ;
    j["action"] = "set_parameter" ;
    j["componentId"] = componentId ;
    j["parameter"] = static_cast<int>(p);
    j["value"] = ParameterValueToJson(v) ;
    
    ApiClient::instance()->sendMessage(j); 
}

void ComponentManager::requestCollectionUpdate(CollectionRequest req){
    json obj = req ;
    ApiClient::instance()->sendMessage(obj); 
}

void ComponentManager::requestModulationDepthUpdate(int componentId, ParameterType p, double depth){
    json obj ;
    obj["action"] = "set_modulation_depth" ;
    obj["componentId"] = componentId ;
    obj["parameter"] = GET_PARAMETER_TRAIT_MEMBER(p, name);
    obj["depth"] = depth ;

    ApiClient::instance()->sendMessage(obj);
}

void ComponentManager::requestModulationStrategyUpdate(int componentId, ParameterType p, ModulationStrategy strategy){
    json obj ;
    obj["action"] = "set_modulation_strategy" ;
    obj["componentId"] = componentId ;
    obj["parameter"] = GET_PARAMETER_TRAIT_MEMBER(p, name);
    obj["strategy"] = static_cast<int>(strategy) ;

    ApiClient::instance()->sendMessage(obj);
}

void ComponentManager::requestModelSync(int componentId){
    json obj ;
    obj["action"] = "sync_component" ;
    obj["componentId"] = componentId ;
    
    ApiClient::instance()->sendMessage(obj);
}

void ComponentManager::renameComponent(int id, const QString& name){
    ///
}

void ComponentManager::renameGroup(int id, const QString& name){
    ///
}

ComponentModel* ComponentManager::getModel(int componentId) const {
    if ( !models_.contains(componentId) ){
        qWarning() << "No component model found with id = " << componentId ;
        return nullptr ;
    } 

    return models_.at(componentId) ;
}

ComponentParameters* ComponentManager::getParameters(int componentId) const {
    if ( !parameters_.contains(componentId) ){
        qWarning() << "No component editor found with id = " << componentId ;
        return nullptr ;
    } 

    return parameters_.at(componentId) ;
}

ModulationParameters* ComponentManager::getModulationParameters(int componentId) const {
    if ( !modParameters_.contains(componentId) ){
        qWarning() << "No modulation editor found with id = " << componentId ;
        return nullptr ;
    } 

    return modParameters_.at(componentId);
}

void ComponentManager::showParameters(int componentId){
    // auto it = parameters_.find(componentId);
    // if ( it == parameters_.end() ){
    //     qWarning() << "requested editor for invalid component id:" << componentId ;
    //     return ;
    // }
    // requestModelSync(componentId);
    // it->second->move(QCursor::pos());
    // it->second->open();
    // it->second->raise();
}

void ComponentManager::showModulation(int componentId){
    // auto it = modParameters_.find(componentId);
    // if ( it == modParameters_.end() ){
    //     qWarning() << "requested modulation editor for invalid component id:" << componentId ;
    //     return ;
    // }
    // requestModelSync(componentId);
    // it->second->show();
    // it->second->raise();
}

void ComponentManager::showGroupParameters(int componentId){}

void ComponentManager::showGroupModulation(int componentId){}

int ComponentManager::createGroup(const std::vector<int> componentIds, bool block){
    // int id = currentGroupId_++ ;
    // QString name = QString("Group %1").arg(id);

    // GroupModel* m = new GroupModel(id);
    // GroupEditor* g = new GroupEditor(name, mainWindow_);
    // ModulationParameters* me = new ModulationParameters(name, mainWindow_);
    // groupModels_[id] = m ;
    // groupEditors_[id] = g ;
    // groupModulationEditors_[id] = me ;

    // connect(
    //     g, &GroupEditor::parameterEdited,
    //     this, &ComponentManager::onParameterEdited
    // );

    // connect(
    //     me, &ModulationParameters::modulationDepthEdited,
    //     this, &ComponentManager::onModulationDepthEdited
    // );

    // connect(
    //     me, &ModulationParameters::modulationStrategyEdited,
    //     this, &ComponentManager::onModulationStrategyEdited
    // );

    // for ( auto i : componentIds ){
    //     auto model = getModel(i);
    //     m->addComponent(model);
    //     g->addComponent(model);

    //     const ComponentDescriptor& d = model->getDescriptor();
    //     for ( const auto& p : d.modulatableParameters ){
    //         me->add(model->getModulationModel(p));
    //     }
        
    //     // handle specialized collection widgets
    //     auto cw = getCollectionWidget(g->getComponentParameters(i));
    //     if ( cw ){
    //         connect(
    //             cw, &CollectionWidget::collectionEdited,
    //             this, &ComponentManager::onCollectionEdited
    //         );
    //     }
    // }

    // if ( ! block ){
    //     emit componentGroupCreated(id, componentIds);
    // }
    
    // return id ;
}

void ComponentManager::appendToGroup(int groupId, const std::vector<int> componentIds){
    // auto model = getGroupModel(groupId);
    // auto editor = getGroupEditor(groupId);
    // auto modParams = getGroupModulationEditor(groupId);

    // if ( !model || !editor || !modParams ){
    //     qWarning() << "Could not find group editor with Id " << groupId ;
    //     return ;
    // }
    
    // for ( auto id : componentIds ){
    //     auto m = getModel(id);

    //     model->addComponent(m);
    //     editor->addComponent(m);

    //     const ComponentDescriptor& d = m->getDescriptor();
    //     for ( const auto& p : d.modulatableParameters ){
    //         modParams->add(m->getModulationModel(p));
    //     }
    // }

    // emit componentGroupUpdated(groupId, model->getComponents());
}

void ComponentManager::removeGroup(int groupId){
    // auto model = getGroupModel(groupId);
    // auto editor = getGroupEditor(groupId);
    // auto modParams = getGroupModulationEditor(groupId);

    // if ( !model || !editor || !modParams ) return ;

    // emit componentGroupRemoved(groupId, model->getComponents());

    // groupModels_.erase(groupId);
    // groupEditors_.erase(groupId);
    // groupModulationEditors_.erase(groupId);
    
    // model->deleteLater();
    // editor->deleteLater();    
    // modParams->deleteLater();
}

void ComponentManager::addComponent(int componentId, ComponentType type){
    auto model = new ComponentModel(componentId, type);
    models_[componentId] = model ;

    auto params = new ComponentParameters(model);
    parameters_[componentId] = params ;

    auto modParams = new ModulationParameters(model);
    modParameters_[componentId] = modParams ;

    // handle edit communications
    connect(
        parameters_[componentId], &ComponentParameters::parameterEdited,
        this, &ComponentManager::onParameterEdited
    );

    connect(
        modParams, &ModulationParameters::modulationDepthEdited,
        this, &ComponentManager::onModulationDepthEdited
    );

    connect(
        modParams, &ModulationParameters::modulationStrategyEdited,
        this, &ComponentManager::onModulationStrategyEdited
    );
    
    // // handle collection widget if exists
    // auto cw = getCollectionWidget(editor->getComponentParameters());
    // if ( cw ){
    //     connect(
    //         cw, &CollectionWidget::collectionEdited,
    //         this, &ComponentManager::onCollectionEdited
    //     );
    // }

    emit componentAdded(componentId, type);
}

void ComponentManager::removeComponent(int componentId){
    auto it = models_.find(componentId);

    if ( it == models_.end() ){
        qWarning() << "could not find model for component to delete";
        return ;
    }

    models_[componentId]->deleteLater();
    parameters_[componentId]->deleteLater();
    modParameters_[componentId]->deleteLater();
    models_.erase(componentId);
    parameters_.erase(componentId);
    modParameters_.erase(componentId);

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
        qWarning() << "Cannot find model with id = " << id ;
        return ;
    }

    if ( data.contains("parameters") ){
        for ( const auto& [p, obj] : data.at("parameters").items() ){
            if ( ! obj.contains("currentValue") ) continue ;
            setParameterValue(id, parameterFromString(p), obj.at("currentValue") );
        }
    }
}

CollectionWidget* ComponentManager::getCollectionWidget(ComponentParameters* params) const {
    if ( !params ) return nullptr ;
    auto specialized = params->getSpecializedWidget();
    if ( !specialized ) return nullptr ;

    CollectionWidget* cw = dynamic_cast<CollectionWidget*>(specialized);
    if ( !cw ){
        cw = specialized->findChild<CollectionWidget*>();
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
        qWarning() << "Could not find model with Component ID" << req.componentId 
            << ". Will not process collection request" ;
        return true ;
    }
    
    // auto cw = getCollectionWidget(it->second->getComponentParameters());
    // if ( cw ){
    //     cw->updateCollection(req);
    // }
    
    return true ;
}


void ComponentManager::onApiDataReceived(const json& msg){
    QString action = QString::fromStdString(msg["action"]);
    bool success = msg.at("status") == "success" ;

    if ( action == "add_component" && success ){
        int id = msg.at("componentId");
        ComponentType type = static_cast<ComponentType>(msg.at("type"));
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

    if ( action == "set_component_parameter" && success ){
        int id = msg.at("componentId");
        auto it = models_.find(id);
        if ( it == models_.end() ){
            qWarning() << "Could not find model with Component ID" << id 
                << ". Will not process set parameter request" ;
            return ;
        }

        ParameterType p = static_cast<ParameterType>(msg.at("parameter"));

        setParameterValue(id, p, msg.at("value"));
        return ;
    }

    if ( action == "set_modulation_depth" && success ){
        int id = msg["componentId"];
        auto it = models_.find(id);
        if ( it == models_.end() ){
            qWarning() << "Could not find model with Component ID" << id 
                << ". Will not process modulation depth request" ;
            return ;
        }

        ParameterType p = parameterFromString(msg.at("parameter"));
        ModulationModel* m = it->second->getModulationModel(p);
        if ( !m ){
            qWarning() << "Could not find Modulation Model for Parameter " 
                << GET_PARAMETER_TRAIT_MEMBER(p, name) 
                << "from Component Model: " << it->second ;
            return ;
        }

        m->setDepth(msg.at("depth"));
        return ;
    }

    if ( action == "set_modulation_strategy" && success ){
        int id = msg.at("componentId");
        auto model = getModel(id);
        if (  !model ){
            qWarning() << "Could not find model with Component ID" << id 
                << ". Will not process modulation strategy request" ;
            return ;
        }

        ParameterType p = parameterFromString(msg.at("parameter"));
        ModulationModel* m = model->getModulationModel(p);
        if ( !m ){
            qWarning() << "Could not find Modulation Model for Parameter " 
                << GET_PARAMETER_TRAIT_MEMBER(p, name) 
                << "from Component Model: " << id ;
            return ;
        }

        m->setStrategy(static_cast<ModulationStrategy>(msg.at("strategy")));
        return ;
    }

    if ( success && handleCollectionApiResponse(msg) ){
        return ;
    }
}

void ComponentManager::onParameterEdited(int componentId, ParameterType p, ParameterValue value){
    requestParameterUpdate(componentId, p, value);
}

void ComponentManager::onCollectionEdited(CollectionRequest req ){
    qDebug() << "collection edit request received!";
    requestCollectionUpdate(req);
}

void ComponentManager::onModulationDepthEdited(int componentId, ParameterType p, double depth){
    requestModulationDepthUpdate(componentId, p, depth);
}

void ComponentManager::onModulationStrategyEdited(int componentId, ParameterType p, ModulationStrategy strategy){
    requestModulationStrategyUpdate(componentId, p, strategy);
}

void ComponentManager::onConnectionAdded(const ConnectionRequest& req){
    if ( 
        req.inboundSocket == SocketType::ModulationInbound && 
        req.inboundID.has_value() &&
        req.inboundParameter.has_value()
    ){
        auto modParams = getModulationParameters(req.inboundID.value());
        if ( !modParams ) return ;
        modParams->setConnectionStatus(req.inboundParameter.value(), true);
    }
}

void ComponentManager::onConnectionRemoved(const ConnectionRequest& req){
    if ( 
        req.inboundSocket == SocketType::ModulationInbound && 
        req.inboundID.has_value() &&
        req.inboundParameter.has_value()
    ){
        auto modParams = getModulationParameters(req.inboundID.value());
        if ( !modParams ) return ;
        modParams->setConnectionStatus(req.inboundParameter.value(), false);
    }
}