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

#ifndef BUFFER_REVERSER_HPP_
#define BUFFER_REVERSER_HPP_

#include "configs/BufferReverserConfig.hpp"
#include "core/Node.hpp"
#include "interfaces/BufferProcessor.hpp"
#include "interfaces/ComponentInfo.hpp"

class BufferReverser : 
    public Node,
    public ComponentInfo,
    public BufferProcessor
{
    
public:
    BufferReverser(int id, BufferReverserConfig cfg);

    BufferReverser(const BufferReverser&) = delete ;
    BufferReverser& operator=(const BufferReverser&) = delete ;
    BufferReverser(BufferReverser&&) = delete ;
    BufferReverser& operator=(BufferReverser&&) = delete ;

protected:
    void process(BufferAccessor& accessor) override ;

};

#endif // BUFFER_REVERSER_HPP_