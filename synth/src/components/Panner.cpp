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
#include "capabilities/AudioIO.hpp"    // IWYU pragma: keep
#include "capabilities/Parameters.hpp" // IWYU pragma: keep

Panner::Panner(int id, PannerConfig):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::Panner),
    AudioProcessor(),
    pan_(createSlotAccessor<ParameterType::PAN>())
{
    addCapability<Capability::AudioIO>(*this, 1, 2);

    auto& params = addCapability<Capability::Parameters>();
    params.addUnitParameter(ParameterConfig(ParameterType::PAN));
    params.bindSlot<ParameterType::PAN>(pan_, 0);

    params.freezeParameterCreation();
}


void Panner::processFrame(const AudioAccessor& accessor, size_t frame) noexcept {
    double input = accessor.input(0)[frame];
    double pan = pan_.modulatedValue();
    double v = pan / 2.0 + 0.5 ;

    accessor.output(0)[frame] = v * input ;
    accessor.output(1)[frame] = (1 - v) * input ;
}