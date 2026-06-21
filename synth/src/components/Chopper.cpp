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

#include "components/Chopper.hpp"
#include "params/ParameterMap.hpp"

Chopper::Chopper(ComponentId id,[[maybe_unused]] ChopperConfig cfg):
    BaseComponent(id, ComponentType::Chopper),
    AudioBufferComponent(1,1)
{
    parameters_->add<ParameterType::START_POSITION>(0,false,0,0);
    parameters_->add<ParameterType::DURATION>(0, false, 0, 0);

    parameters_->getParameter<ParameterType::START_POSITION>()->addListener(this);
    parameters_->getParameter<ParameterType::DURATION>()->addListener(this);
}

void Chopper::onInputConnect(){
    onInputUpdated();
}

void Chopper::onInputDisconnect(){
    onInputUpdated();
}

void Chopper::onInputUpdated(){
    auto start = parameters_->getParameter<ParameterType::START_POSITION>();
    auto duration = parameters_->getParameter<ParameterType::DURATION>();

    const auto& inp = getInputs(0);
    if ( inp.size() > 0 ){
        BufferConnection conn = *inp.begin();
        const auto& buf = conn.component->getBuffer(conn.index);    

        start->setMaximum(buf.size() - 1);
        start->setValue(0, false); // don't recalculate yet
        duration->setMaximum(buf.size() - 1);
        duration->setValue(buf.size() - 1);
    } else {
        start->setMaximum(0);
        start->setValue(0, false);
        duration->setMaximum(0);
        duration->setValue(0);
    }

    triggerComponentSync();
}

void Chopper::onParameterChanged(ParameterType p){
    switch(p){
    case ParameterType::START_POSITION:
    case ParameterType::DURATION:
        break ;
    default:
        return ;
    }

    const auto& inp = getInputs(0);
    if ( inp.size() == 0 ) return ;
        
    BufferConnection conn = *inp.begin();
    const auto& buf = conn.component->getBuffer(conn.index);    

    // guard against empty inbound buffer
    if ( buf.empty() ){
        buffers_[0].clear();
        notifyDownstream(0);
        return ;
    }

    int start    = parameters_->getParameter<ParameterType::START_POSITION>()->getValue();
    int duration = parameters_->getParameter<ParameterType::DURATION>()->getValue();

    // clamp duration
    int maxDuration = int(buf.size()) - start ;
    if ( duration <= 0 || duration > maxDuration ){
        duration = maxDuration ;
        parameters_->getParameter<ParameterType::DURATION>()->setValue(duration, false);
    }

    buffers_[0].assign(
        buf.begin() + start,
        buf.begin() + start + duration
    );
    notifyDownstream(0);
}