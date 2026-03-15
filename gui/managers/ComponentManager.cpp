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
    editors_(),
    groupModels_(),
    groupEditors_()
{
    connect(
        ApiClient::instance(), &ApiClient::dataReceived, 
        this, &ComponentManager::onApiDataReceived
    );
}

ComponentManager::~ComponentManager(){
    for ( const auto& m : models_ ) m.second->deleteLater();
    for ( const auto& e : editors_ ) e.second->deleteLater();
    for ( const auto& gm: groupModels_ ) gm.second->deleteLater();
    for ( const auto& ge : groupEditors_ ) ge.second->deleteLater();
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

void ComponentManager::renameComponent(int id, const QString& name){
    auto editor = getEditor(id);

    if ( !editor ) return ;
    editor->setName(name);
}

void ComponentManager::renameGroup(int id, const QString& name){
    auto editor = getGroupEditor(id);

    if ( !editor ) return ;
    editor->setName(name);
}

ComponentModel* ComponentManager::getModel(int componentId) const {
    if ( !models_.contains(componentId) ){
        qWarning() << "No component model found with id = " << componentId ;
        return nullptr ;
    } 

    return models_.at(componentId) ;
}

ComponentEditor* ComponentManager::getEditor(int componentId) const {
    if ( !editors_.contains(componentId) ){
        qWarning() << "No component editor found with id = " << componentId ;
        return nullptr ;
    } 

    return editors_.at(componentId) ;
}

ModulationEditor* ComponentManager::getModulationEditor(int componentId) const {
    if ( !modulationEditors_.contains(componentId) ){
        qWarning() << "No modulation editor found with id = " << componentId ;
        return nullptr ;
    } 

    return modulationEditors_.at(componentId) ;
}

GroupModel* ComponentManager::getGroupModel(int groupId) const {
    if ( !groupModels_.contains(groupId) ){
        qWarning() << "No group model found with id = " << groupId ;
        return nullptr ;
    } 

    return groupModels_.at(groupId) ;
}

GroupEditor* ComponentManager::getGroupEditor(int groupId) const {
    if ( !groupEditors_.contains(groupId) ){
        qWarning() << "No group editor found with id = " << groupId ;
        return nullptr ;
    }

    return groupEditors_.at(groupId) ;
}

ModulationEditor* ComponentManager::getGroupModulationEditor(int groupId) const {
    if ( !groupModulationEditors_.contains(groupId) ){
        qWarning() << "No group modulation editor found with id = " << groupId ;
        return nullptr ;
    }

    return groupModulationEditors_.at(groupId) ;
}

void ComponentManager::showEditor(int componentId){
    auto it = editors_.find(componentId);
    if ( it == editors_.end() ){
        qWarning() << "requested editor for invalid component id:" << componentId ;
        return ;
    }
    it->second->show();
    it->second->raise();
}

void ComponentManager::showModulationEditor(int componentId){
    auto it = modulationEditors_.find(componentId);
    if ( it == modulationEditors_.end() ){
        qWarning() << "requested modulation editor for invalid component id:" << componentId ;
        return ;
    }
    it->second->show();
    it->second->raise();
}

void ComponentManager::showGroupEditor(int groupId){
    auto it = groupEditors_.find(groupId);
    if ( it == groupEditors_.end() ){
        qWarning() << "requested group editor for invalid group id:" << groupId ;
        return ;
    }
    it->second->show();
    it->second->raise();
}

void ComponentManager::showGroupModulationEditor(int groupId){
    auto it = groupModulationEditors_.find(groupId);
    if ( it == groupModulationEditors_.end() ){
        qWarning() << "requested group editor for invalid group id:" << groupId ;
        return ;
    }
    it->second->show();
    it->second->raise();
}

void ComponentManager::createGroup(const std::vector<int> componentIds){
    int id = currentGroupId_++ ;
    QString name = QString("Group %1").arg(id);

    GroupModel* m = new GroupModel(id);
    GroupEditor* g = new GroupEditor(name);
    ModulationEditor* me = new ModulationEditor(name);
    groupModels_[id] = m ;
    groupEditors_[id] = g ;
    groupModulationEditors_[id] = me ;

    connect(
        g, &GroupEditor::parameterEdited,
        this, &ComponentManager::onParameterEdited
    );

    connect(
        me, &ModulationEditor::modulationDepthEdited,
        this, &ComponentManager::onModulationDepthEdited
    );

    connect(
        me, &ModulationEditor::modulationStrategyEdited,
        this, &ComponentManager::onModulationStrategyEdited
    );

    for ( auto i : componentIds ){
        auto model = getModel(i);
        m->addComponent(model);
        g->addComponent(model);

        const ComponentDescriptor& d = model->getDescriptor();
        for ( const auto& p : d.modulatableParameters ){
            me->add(model->getModulationModel(p));
        }
        
        // handle specialized collection widgets
        auto cw = getCollectionWidget(g->getComponentParameters(i));
        if ( cw ){
            connect(
                cw, &CollectionWidget::collectionEdited,
                this, &ComponentManager::onCollectionEdited
            );
        }
    }

    emit componentGroupCreated(id, componentIds);
}

void ComponentManager::appendToGroup(int groupId, const std::vector<int> componentIds){
    auto model = getGroupModel(groupId);
    auto editor = getGroupEditor(groupId);
    auto modEditor = getGroupModulationEditor(groupId);

    if ( !model || !editor || !modEditor ){
        qWarning() << "Could not find group editor with Id " << groupId ;
        return ;
    }
    
    for ( auto id : componentIds ){
        auto m = getModel(id);

        model->addComponent(m);
        editor->addComponent(m);

        const ComponentDescriptor& d = m->getDescriptor();
        for ( const auto& p : d.modulatableParameters ){
            modEditor->add(m->getModulationModel(p));
        }
    }

    emit componentGroupUpdated(groupId, model->getComponents());
}

void ComponentManager::removeGroup(int groupId){
    auto model = getGroupModel(groupId);
    auto editor = getGroupEditor(groupId);
    auto modEditor = getGroupModulationEditor(groupId);

    if ( !model || !editor || !modEditor ) return ;

    emit componentGroupRemoved(groupId, model->getComponents());

    groupModels_.erase(groupId);
    groupEditors_.erase(groupId);
    groupModulationEditors_.erase(groupId);
    
    model->deleteLater();
    editor->deleteLater();    
    modEditor->deleteLater();
}

json ComponentManager::serialize() const {

}

void ComponentManager::addComponent(int componentId, ComponentType type){
    auto model = new ComponentModel(componentId, type);
    models_[componentId] = model ;

    auto editor = new ComponentEditor(model);
    editors_[componentId] = editor ;

    auto modEditor = new ModulationEditor(editor->getName());
    modulationEditors_[componentId] = modEditor ;

    const ComponentDescriptor& d = model->getDescriptor();
    for ( const auto& p : d.modulatableParameters ){
        modEditor->add(model->getModulationModel(p));
    }

    // handle editor communications
    connect(
        editors_[componentId], &ComponentEditor::parameterEdited,
        this, &ComponentManager::onParameterEdited
    );

    connect(
        modEditor, &ModulationEditor::modulationDepthEdited,
        this, &ComponentManager::onModulationDepthEdited
    );

    connect(
        modEditor, &ModulationEditor::modulationStrategyEdited,
        this, &ComponentManager::onModulationStrategyEdited
    );
    
    // handle collection widget if exists
    auto cw = getCollectionWidget(editor->getComponentParameters());
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
        qWarning() << "could not find model for component to delete";
        return ;
    }

    models_[componentId]->deleteLater();
    editors_[componentId]->deleteLater();
    modulationEditors_[componentId]->deleteLater();
    models_.erase(componentId);
    editors_.erase(componentId);
    modulationEditors_.erase(componentId);

    emit componentRemoved(componentId);
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

    auto it = editors_.find(req.componentId);
    if ( it == editors_.end() ){
        qWarning() << "Could not find model with Component ID" << req.componentId 
            << ". Will not process collection request" ;
        return true ;
    }
    
    auto cw = getCollectionWidget(it->second->getComponentParameters());
    if ( cw ){
        cw->updateCollection(req);
    }
    
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

    if ( action == "set_component_parameter" && success ){
        int id = msg.at("componentId");
        auto it = models_.find(id);
        if ( it == models_.end() ){
            qWarning() << "Could not find model with Component ID" << id 
                << ". Will not process set parameter request" ;
            return ;
        }

        ParameterType p = static_cast<ParameterType>(msg.at("parameter"));
        
        // dispatch to set parameter value with correct variant
        ParameterValue v ; 
        switch(p){
        #define X(name) \
        case ParameterType::name: \
            v = static_cast<GET_PARAMETER_VALUE_TYPE(ParameterType::name)>(msg["value"]); \
            break ; 
        PARAMETER_TYPE_LIST
        #undef X     
        default:
            break ;
        }
        it->second->setParameterValue(p, v);
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
        auto it = models_.find(id);
        if ( it == models_.end() ){
            qWarning() << "Could not find model with Component ID" << id 
                << ". Will not process modulation strategy request" ;
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
        auto editor = getModulationEditor(req.inboundID.value());
        if ( !editor ) return ;
        editor->setModulationStatus(req.inboundID.value(), req.inboundParameter.value(), true);
    }
}

void ComponentManager::onConnectionRemoved(const ConnectionRequest& req){
    if ( 
        req.inboundSocket == SocketType::ModulationInbound && 
        req.inboundID.has_value() &&
        req.inboundParameter.has_value()
    ){
        auto editor = getModulationEditor(req.inboundID.value());
        if ( !editor ) return ;
        editor->setModulationStatus(req.inboundID.value(), req.inboundParameter.value(), false);
    }
}