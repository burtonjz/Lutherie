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

#ifndef __MIDI_FILTER_HPP_
#define __MIDI_FILTER_HPP_

#include "core/Node.hpp"
#include "interfaces/ComponentInfo.hpp"
#include "interfaces/MidiProcessor.hpp"
#include "interfaces/ParameterAccessor.hpp"
#include "configs/MidiFilterConfig.hpp"

class MidiFilter :
    public Node,
    public ComponentInfo,
    public MidiProcessor
{
private:
    ParameterAccessor midiValue_ ;

    // MIDI PROCESSOR
    void handleKeyPressed(MidiNote note, size_t ch = 0) override ;
    void handleKeyReleased(MidiNote note, size_t ch = 0) override ;

public:
    MidiFilter(int id, MidiFilterConfig cfg);

private:
    bool passNote(uint8_t midi) const ;
};

#endif // __MIDI_FILTER_HPP_
