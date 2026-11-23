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

#include "components/Multiply.hpp"
#include "capabilities/AudioIO.hpp"    // IWYU pragma: keep
#include "capabilities/Parameters.hpp" // IWYU pragma: keep
#include "types/ComponentType.hpp"

Multiply::Multiply(int id, MultiplyConfig):
    Node(id, NodeType::COMPONENT)
{
    addCapability<Capability::AudioIO>(*this, 1, 1);
    
    auto& params = addCapability<Capability::Parameters>();
    params.addUnitParameter(ParameterConfig(ParameterType::SCALAR));
    params.bindSlot<ParameterType::SCALAR>(scalar_, 0);
}

ComponentType Multiply::componentType() const { 
    return ComponentType::Multiply ; 
}

void Multiply::processFrame(const AudioAccessor& accessor, size_t frame) noexcept {
    double input = accessor.input(0)[frame];
    accessor.output(0)[frame] = input * scalar_.modulatedValue();
}