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

#ifndef PANNER_HPP_
#define PANNER_HPP_

#include "core/Node.hpp"
#include "interfaces/AudioProcessor.hpp"
#include "interfaces/ComponentInfo.hpp"
#include "configs/PannerConfig.hpp"
#include "params/Parameter.hpp"
#include "types/ParameterType.hpp"

class Panner :
    public Node,
    public ComponentInfo,
    public AudioProcessor
{
private:
    Parameter<ParameterType::PAN>* pan_ ;

public:
    Panner(int id, PannerConfig cfg);

    Panner(const Panner&) = delete ;
    Panner& operator=(const Panner&) = delete ;
    Panner(Panner&&) = delete ;
    Panner& operator=(Panner&&) = delete ;

    // Component Info
    ComponentType componentType() const override ;

    // AudioProcessor
    void processFrame(const AudioAccessor& accessor, size_t frame) noexcept override ;
    // void onPrepare(AudioContext ctx) override ;
    // void onTick(size_t newFrame) override ;
    
};

#endif // PANNER_HPP_