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

#include "managers/GroupManager.hpp"

#include <QWidget>
#include <spdlog/spdlog.h>

GroupManager::GroupManager(QObject* parent):
    QObject(parent),
    currentGroupId_(0),
    groups_(),
    parameterContent_(),
    modulationContent_()
{

}

GroupManager::~GroupManager(){
    const auto copy = groups_ ;
    for ( auto [id, g] : copy ){
        onRequestGroupRemove(id);
    }
}

GroupModel* GroupManager::getModel(int groupId) const {
    if ( !groups_.contains(groupId) ) return nullptr ;
    return groups_.at(groupId);
}

GroupModel* GroupManager::getComponentGroup(int componentId) const {
    for ( auto [id, g] : groups_ ){
        if ( g->hasComponent(componentId) ) return g ;
    }
    return nullptr ;
}

QWidget* GroupManager::getParameters(int groupId) const {
    if ( parameterContent_.contains(groupId) ){
        return parameterContent_.at(groupId);
    }
    return nullptr ;
}

QWidget* GroupManager::getModulationParameters(int groupId) const {
    if ( modulationContent_.contains(groupId) ){
        return modulationContent_.at(groupId);
    }
    return nullptr ;
}

void GroupManager::setParameters(int groupId, QWidget* content){
    parameterContent_[groupId] = content ;
}

void GroupManager::setModulationParameters(int groupId, QWidget* content){
    modulationContent_[groupId] = content ;
}

void GroupManager::removeContent(int groupId){
    if ( parameterContent_.contains(groupId) ){
        parameterContent_.at(groupId)->deleteLater();
        parameterContent_.erase(groupId);
    }
    if ( modulationContent_.contains(groupId) ){
        modulationContent_.at(groupId)->deleteLater();
        modulationContent_.erase(groupId);
    }
}

void GroupManager::onRequestGroupCreate(std::vector<int> componentIds, std::optional<json> deserialize){
    int groupId = currentGroupId_++ ;
    auto model = new GroupModel(groupId, QString("Group %1").arg(groupId));
    for ( const auto id : componentIds ){
        if ( !getComponentGroup(id) ){
            model->addComponent(id);
        }
    }

    if ( model->getComponents().size() == 0 ){
        model->deleteLater();
        SPDLOG_WARN("group creation does not have any valid component ids");
        return ;
    } 

    groups_[groupId] = model ;
    emit groupCreated(groupId, model->getComponents(), deserialize);
}

void GroupManager::onRequestGroupUpdate(int groupId, std::vector<int> componentIds){
    if ( ! groups_.contains(groupId) ){
        SPDLOG_WARN("group update requested for invalid group id: {}", groupId);
        return ;
    }
    auto model = groups_.at(groupId);
    for ( const auto id : componentIds ){
        if ( !getComponentGroup(id) ){
            model->addComponent(id);
        }
    }

    emit groupUpdated(groupId, model->getComponents());

}

void GroupManager::onRequestGroupRemove(int groupId){
    if ( ! groups_.contains(groupId) ){
        SPDLOG_WARN("group removal requested for invalid group id: {}", groupId);
        return ;
    }
    emit groupRemoved(groupId, groups_.at(groupId)->getComponents());

    groups_.at(groupId)->deleteLater();
    groups_.erase(groupId);
}