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

#include "components/BufferReverser.hpp"
#include "capabilities/BufferIO.hpp" // IWYU pragma: keep
#include "interfaces/ComponentInfo.hpp"
#include "status/BufferStatus.hpp"
#include "types/NodeType.hpp"

BufferReverser::BufferReverser(int id, [[maybe_unused]] BufferReverserConfig cfg):
    Node(id, NodeType::COMPONENT),
    ComponentInfo(ComponentType::BufferReverser),
    BufferProcessor()
{
    addCapability<Capability::BufferIO>(*this, 1, 1);
}

void BufferReverser::process(BufferAccessor& accessor){
    auto input = accessor.inputs(0);
    size_t size = input.size();

    accessor.output(0).resize(size);

    if ( size > 0 ){
        accessor.output(0).assign(input.rbegin(), input.rend());
    }

    accessor.setAllStatus(BufferStatus::Ready);
}
