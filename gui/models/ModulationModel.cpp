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

#include "models/ModulationModel.hpp"

ModulationModel::ModulationModel(int id, ParameterType p):
    id_(id),
    parameter_(p),
    depth_(1.0),
    strategy_(GET_PARAMETER_MODULATION_STRATEGY(p,ModulatorRange::UNKNOWN)),
    isConnected_(false),
    connectedId_(-1)
{}

int ModulationModel::getId() const {
    return id_ ;
}

ParameterType ModulationModel::getType() const {
    return parameter_ ;
}

double ModulationModel::getDepth() const {
    return depth_ ;
}

void ModulationModel::setDepth(double depth, bool block){
    depth_ = depth ;

    if ( !block ){
        emit modulationDepthChanged(id_, parameter_, depth_);
    }
}

ModulationStrategy ModulationModel::getStrategy() const {
    return strategy_ ;
}

void ModulationModel::setStrategy(ModulationStrategy strat, bool block){
    strategy_ = strat ;

    if ( !block ){
        emit modulationStrategyChanged(id_, parameter_, strategy_);
    }
}

void ModulationModel::sync() const {
    emit modulationStrategyChanged(id_, parameter_, strategy_);
}