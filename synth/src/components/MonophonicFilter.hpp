/*
 * Copyright (C) 2025 Jared Burton
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

#ifndef MONOPHONIC_FILTER_HPP_
#define MONOPHONIC_FILTER_HPP_

#include "core/Node.hpp"
#include "interfaces/ComponentInfo.hpp"
#include "interfaces/MidiProcessor.hpp"
#include "configs/MonophonicFilterConfig.hpp"

#include <vector>

class MonophonicFilter :
    public Node,    
    public ComponentInfo,
    public MidiProcessor
{
    private:
        std::vector<uint8_t> noteStack_ ;
    
    public:
        MonophonicFilter(int id, MonophonicFilterConfig cfg);

        // MIDI PROCESSOR
        void handleKeyPressed(MidiNote note, size_t ch = 0) override ;
        void handleKeyReleased(MidiNote note, size_t ch = 0) override ;

};


#endif // MONOPHONIC_FILTER_HPP_