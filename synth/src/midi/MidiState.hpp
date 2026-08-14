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

#ifndef __MIDI_STATE_HPP_
#define __MIDI_STATE_HPP_

#include "midi/MidiEventHandler.hpp"
#include "midi/MidiNote.hpp"

#include <spdlog/spdlog.h>

class MidiState {
private:
    std::vector<MidiEventHandler*> handlers_ ; 
    KeyMap notes_ ;
    float pitchbend_ ;

    MidiState();

public:
    static MidiState* instance();

    MidiState(const MidiState&) = delete ;
    MidiState& operator=(const MidiState&) = delete ;
    MidiState(MidiState&&) = delete ;
    MidiState& operator=(MidiState&&) = delete ;

    void addHandler(MidiEventHandler* handler);
    void removeHandler(MidiEventHandler* handler);
    const std::vector<MidiEventHandler*>& getHandlers() const ;
    
    // getters/setters
    const MidiNote* getNote(int midiNote);

    // processing messages
    void processMsgNoteOn(int midiNote, int velocity);
    void processMsgNoteOff(int midiNote, int velocity);
    void processMsgPitchbend(float pitchbend);
    void processMsgNotePressure([[maybe_unused]] int midiNote, [[maybe_unused]] int pressure);
    void processMsgControl(int ctrlID, int ctrlValue);
    void processMsgProgram([[maybe_unused]] int program);
    void processMsgChannelPressure([[maybe_unused]] int pressure);

    void reset();

};

#endif // __MIDI_STATE_HPP_