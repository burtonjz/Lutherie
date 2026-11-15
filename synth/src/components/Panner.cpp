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
#include "capabilities/AudioIO.hpp"
#include "capabilities/Parameters.hpp"
#include "types/ComponentType.hpp"

Panner::Panner(int id, PannerConfig cfg):
    Node(id, NodeType::COMPONENT)
{
    addCapability<Capability::AudioIO>(*this, 1, 2);

    auto& params = addCapability<Capability::Parameters>();
    pan_ = &params.add<ParameterType::PAN>(cfg.pan, true);
}
    
ComponentType Panner::componentType() const {
    return ComponentType::Panner ;
}

void Panner::processFrame(const AudioAccessor& accessor, size_t frame) noexcept {
    double input = accessor.input(0)[frame];
    double pan = pan_->getInstantaneousValue();
    double v = pan / 2.0 + 0.5 ;

    accessor.output(0)[frame] = v * input ;
    accessor.output(1)[frame] = (1 - v) * input ;
}