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
#include "capabilities/BufferIO.hpp" // IWYU pragma: keep
#include "capabilities/Parameters.hpp" // IWYU pragma: keep
#include "meta/ComponentRegistry.hpp"
#include "types/ComponentType.hpp"

Chopper::Chopper(int id, ChopperConfig):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::Chopper),
    BufferProcessor(),
    ParameterObserver(),
    sample_(createParameterAccessor()),
    begin_(createSlotAccessor<ParameterType::SAMPLE>()),
    end_(createSlotAccessor<ParameterType::SAMPLE>())
{
    auto* buf = &addCapability<Capability::BufferIO>(*this, 1, 1);
    trigger_ = buf ;

    auto& params = addCapability<Capability::Parameters>(this);

    params.addCollection(
        ComponentRegistry
            ::getComponentDescriptor(ComponentType::Chopper)
            .collection.value()
    );

    params.bindParameter(sample_, ParameterType::SAMPLE);
    params.observe(ParameterType::SAMPLE);

    sample_.bindSlot(begin_, 0);
    sample_.bindSlot(end_, 1);

    params.freezeParameterCreation();
}

void Chopper::process(BufferAccessor& accessor){
    static constexpr ParameterType p = ParameterType::SAMPLE ;
    using ValueType = ParameterTraits<p>::ValueType ;

    accessor.setAllStatus(BufferStatus::NotReady);

    const auto& inp = accessor.inputs(0);
    if ( inp.size() > 0 ){
        ValueType lastSample = inp.size() - 1 ;
        sample_.setLimits({0, lastSample});
        begin_.setValue(0, false);
        end_.setValue(lastSample, true);
    } else {
        sample_.setLimits({0, 0});
        begin_.setValue(0, false);
        end_.setValue(0, true);
    }
}

void Chopper::onTrigger(BufferAccessor& accessor){
    const auto& inp = accessor.inputs(0);
    
    if ( inp.size() == 0 ){
        SPDLOG_WARN("parameter update received, but inbound buffer is empty.");
        accessor.output(0).clear();
        accessor.setAllStatus(BufferStatus::Ready);
        return ;
    } else {
        accessor.output(0).resize(inp.size());
    }

    int start =  begin_.value();
    int end = end_.value();
    accessor.output(0).assign(
        inp.begin() + start,
        inp.begin() + end
    );

    SPDLOG_DEBUG(
        "Chopper buffer updated: startSample={}, endSample={}, size={}", 
        start, end, accessor.output(0).size()
    );
    accessor.setAllStatus(BufferStatus::Ready);
}

void Chopper::onParameterChanged(ParameterType p, uint16_t){
    if ( p != ParameterType::SAMPLE ) return ;
    trigger_->trigger();
}