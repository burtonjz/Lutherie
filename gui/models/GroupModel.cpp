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

#include "models/GroupModel.hpp"
#include <spdlog/spdlog.h>

GroupModel::GroupModel(int id, QString name):
    id_(id),
    name_(name),
    componentIds_()
{}


int GroupModel::getId() const {
    return id_ ;
} 

const QString& GroupModel::getName() const {
    return name_ ;
} 

void GroupModel::setName(QString name){
    name_ = name ;
    emit groupRenamed(id_);
}

void GroupModel::addComponent(int componentId){
    if ( hasComponent(componentId) ){
        SPDLOG_WARN("componentId {} is already present in group model with id = {}",
            componentId, id_
        );
        return ;
    }
    componentIds_.push_back(componentId);
} 

void GroupModel::removeComponent(int componentId){
    componentIds_.erase(std::remove(
        componentIds_.begin(), componentIds_.end(), componentId), 
        componentIds_.end()
    );
} 

bool GroupModel::hasComponent(int componentId){
    return std::find(componentIds_.begin(), componentIds_.end(), componentId) 
        != componentIds_.end() ;
}

const std::vector<int>& GroupModel::getComponents() const {
    return componentIds_ ;
} 