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

#include "core/Node.hpp"
#include "interfaces/AudioProcessor.hpp"
#include "interfaces/ComponentInfo.hpp"
#include "interfaces/ParameterAccessor.hpp"
#include "configs/MultiplyConfig.hpp"
#include "types/ParameterType.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class Multiply : 
    public Node, 
    public ComponentInfo ,
    public AudioProcessor 
{
private:
    ParameterAccessor<ParameterType::SCALAR> scalar_ ;

public:
    Multiply(int id, MultiplyConfig cfg);
    
    Multiply(const Multiply&) = delete ;
    Multiply& operator=(const Multiply&) = delete ;
    Multiply(Multiply&&) = delete ;
    Multiply& operator=(Multiply&&) = delete ;

    // Component Info
    ComponentType componentType() const override ;

    // AudioProcessor
    void processFrame(const AudioAccessor& accessor, size_t frame) noexcept override ;
    // void onPrepare(AudioContext ctx) override ;
    // void onTick(size_t newFrame) override ;

};


#endif // GAIN_COMPONENT_HPP_



