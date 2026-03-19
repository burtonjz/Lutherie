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

#include <QDebug>

GroupModel::GroupModel(int id, QString name):
    id_(id),
    name_(name),
    models_()
{}


int GroupModel::getId() const {
    return id_ ;
} 

QString GroupModel::getName() const {
    return name_ ;
} 

void GroupModel::addComponent(ComponentModel* model){
    if ( !model ) return ;

    int id = model->getId() ;

    if ( models_.contains(id) ){
        qWarning() << "group with id " << id << "already exists in group model map." ;
        return ;
    }

    models_[id] = model ;
} 

void GroupModel::removeComponent(int componentId){
    if ( !models_.contains(componentId) ){
        qWarning() << "group with id " << componentId << "does not exist in group model map." ;
        return ;
    }

    models_.erase(componentId);
} 

const std::vector<int> GroupModel::getComponents() const {
    std::vector<int> keys ;
    keys.reserve(models_.size());
    for ( const auto& [k, _] : models_ ){
        keys.push_back(k);
    }
    return keys ;
} 

// ParameterExposure GroupModel::getExposure(int componentId, ParameterType p) const {
    
// } 

// void GroupModel::setExposure(int componentId, ParameterType p, ParameterExposure e){

// } 

// bool GroupModel::isVisible(int componentId, ParameterType p) const {

// } 

// bool GroupModel::isLocked(int componentId, ParameterType p) const {

// } 
