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

#include "Delay.hpp"
#include "config/Config.hpp"
#include "params/ParameterMap.hpp"

Delay::Delay(ComponentId id, DelayConfig cfg):
    BaseComponent(id, ComponentType::Delay),
    AudioStreamComponent(1,1),
    delay_(cfg.max_delay_sec * Config::get<int>("audio.sample_rate").value())
{
    parameters_->add<ParameterType::DELAY>(cfg.delay,true,0,cfg.max_delay_sec * sampleRate_);
    parameters_->add<ParameterType::GAIN>(cfg.gain, true);

}

void Delay::calculateSample(){
    double input = aggregateInputs(0);
    delay_.write(input);

    size_t delay = parameters_->getParameter<ParameterType::DELAY>()->getInstantaneousValue() ; // in samples
    double gain = parameters_->getParameter<ParameterType::GAIN>()->getInstantaneousValue() ;

    setBufferValue(0, delay_.read(delay) * gain) ;
}