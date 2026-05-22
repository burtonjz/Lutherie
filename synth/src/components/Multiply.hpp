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

#ifndef GAIN_COMPONENT_HPP_
#define GAIN_COMPONENT_HPP_

#include "core/AudioSignalComponent.hpp"
#include "configs/MultiplyConfig.hpp"
#include "params/ParameterMap.hpp"

class Multiply : public AudioSignalComponent {
public:
    Multiply(ComponentId id, MultiplyConfig cfg):
        BaseComponent(id,ComponentType::Multiply),
        AudioSignalComponent(1,1)
    {
        parameters_->add<ParameterType::SCALAR>(cfg.scalar, true);
    }

    void calculateSample(){
        double input = aggregateInputs(0);

        double scalar = parameters_->getParameter<ParameterType::SCALAR>()->getInstantaneousValue() ;
        setBufferValue(0, input * scalar) ;
    }
};


#endif // GAIN_COMPONENT_HPP_



