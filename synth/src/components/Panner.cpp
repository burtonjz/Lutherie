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

#include "components/Panner.hpp"
#include "params/ParameterMap.hpp"

Panner::Panner(ComponentId id, PannerConfig cfg):
    BaseComponent(id, ComponentType::Panner),
    AudioStreamComponent(1,2)
{
    parameters_->add<ParameterType::PAN>(cfg.pan,true);
}

void Panner::calculateSample(){
    double input = aggregateInputs(0);
    double pan = parameters_->getParameter<ParameterType::PAN>()->getInstantaneousValue();
    double v = pan / 2.0 + 0.5 ;
    
    setBufferValue(0, v * input);
    setBufferValue(1, ( 1 - v ) * input);
}