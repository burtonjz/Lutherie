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
}

void GroupModel::addComponent(int componentId){
    auto [ it, inserted ] = componentIds_.insert(componentId);
} 

void GroupModel::removeComponent(int componentId){
    componentIds_.erase(componentId);
} 

bool GroupModel::hasComponent(int componentId){
    return componentIds_.contains(componentId);
}

const std::unordered_set<int>& GroupModel::getComponents() const {
    return componentIds_ ;
} 