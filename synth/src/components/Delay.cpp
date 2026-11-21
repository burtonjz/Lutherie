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

#include "components/Delay.hpp"
#include "capabilities/AudioIO.hpp"    // IWYU pragma: keep
#include "capabilities/Parameters.hpp" // IWYU pragma: keep
#include "types/ComponentType.hpp"

Delay::Delay(int id, DelayConfig cfg):
    Node(id, NodeType::COMPONENT),
    maxDelaySeconds_(cfg.max_delay_sec),
    buffer_(0)
{
    addCapability<Capability::AudioIO>(*this, 1, 1);
    
    auto& params = addCapability<Capability::Parameters>();
    params.parameterAccessor<ParameterType::DELAY>(delay_, 0);
}

void Delay::onPrepare(AudioContext ctx){
    using T = ParameterTraits<ParameterType::DELAY>::ValueType ;
    T size = maxDelaySeconds_ * ctx.sampleRate ;

    getCapability<Capability::Parameters>()
        ->definition(ParameterType::DELAY)
        ->setRange({0, size});
    
    buffer_.setCapacity(static_cast<size_t>(size));
}

ComponentType Delay::componentType() const {
    return ComponentType::Delay ;
}

void Delay::processFrame(const AudioAccessor& accessor, size_t frame) noexcept {
    double input = accessor.input(0)[frame];
    buffer_.write(input);

    size_t delay = delay_.modulatedValue();
    accessor.output(0)[frame] = buffer_.read(delay);
}