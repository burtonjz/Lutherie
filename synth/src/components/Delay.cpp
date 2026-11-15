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
#include "capabilities/AudioIO.hpp"
#include "capabilities/Parameters.hpp"
#include "types/ComponentType.hpp"

Delay::Delay(int id, DelayConfig cfg):
    Node(id, NodeType::COMPONENT),
    maxDelaySeconds_(cfg.max_delay_sec),
    buffer_(0)
{
    addCapability<Capability::AudioIO>(*this, 1, 1);
    
    auto& params = addCapability<Capability::Parameters>();
    delay_ = &params.add<ParameterType::DELAY>(cfg.delay, true, 0, maxDelaySeconds_); 
}

void Delay::onPrepare(AudioContext ctx){
    size_t size = maxDelaySeconds_ * ctx.sampleRate ;
    delay_->setMaximum(size);
    buffer_.setCapacity(maxDelaySeconds_ * ctx.sampleRate);
}

ComponentType Delay::componentType() const {
    return ComponentType::Delay ;
}

void Delay::processFrame(const AudioAccessor& accessor, size_t frame) noexcept {
    double input = accessor.input(0)[frame];
    buffer_.write(input);

    size_t delay = delay_->getInstantaneousValue();
    accessor.output(0)[frame] = buffer_.read(delay);
}