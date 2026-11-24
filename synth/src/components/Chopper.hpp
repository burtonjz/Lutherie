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

#ifndef CHOPPER_HPP_
#define CHOPPER_HPP_

#include "core/Node.hpp"
#include "interfaces/BufferProcessor.hpp"
#include "interfaces/ComponentInfo.hpp"
#include "interfaces/ParameterObserver.hpp"
#include "interfaces/ParameterAccessor.hpp"
#include "configs/ChopperConfig.hpp"
class Chopper : 
    public Node,
    public ComponentInfo,
    public BufferProcessor,
    public ParameterObserver
{
private:
    BufferTrigger* trigger_ = nullptr ;

    ParameterAccessor sample_ ;
    SlotAccessor<ParameterType::SAMPLE> begin_ ;
    SlotAccessor<ParameterType::SAMPLE> end_ ;

public:
    Chopper(int id, ChopperConfig cfg);

    Chopper(const Chopper&) = delete ;
    Chopper& operator=(const Chopper&) = delete ;
    Chopper(Chopper&&) = delete ;
    Chopper& operator=(Chopper&&) = delete ;

    // Buffer Processor
    void process(BufferAccessor& accessor) override ;
    void onTrigger(BufferAccessor& accessor) override ;

    // ParameterObserver
    void onParameterChanged(ParameterType p, uint16_t slot) override ;

};

#endif // CHOPPER_HPP_