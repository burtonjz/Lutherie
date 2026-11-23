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

#ifndef MIDI_PROCESSOR_HPP_
#define MIDI_PROCESSOR_HPP_

#include "midi/MidiEventQueue.hpp"
#include <memory>
#include <vector>

// forward declarations
class MidiNote ;
class MidiIO ;

class MidiProcessor {
public:
    virtual ~MidiProcessor() = default ;

    // default behavior -> push to queue
    virtual void handleKeyPressed(MidiNote note, size_t ch = 0);

    // default behavior -> push to queue
    virtual void handleKeyReleased(MidiNote note, size_t ch = 0);

    // on every tick in the Midi Engine, a released note is checked
    // against this function to determine when the note should die.
    // default behavior -> as soon as released.
    virtual bool shouldKillNote(const MidiNote& note, size_t ch = 0) const ;

    // default behavior -> no-op
    virtual void onTick(float dt);

    // default behavior -> no-op
    virtual void onReset();
    
protected:
    bool push(MidiNote note, size_t ch = 0);

private:
    // MidiIO is used for
    // 1. constructing queues (based on number of outputs)
    // 2. pop queue events
    // 3. trigger handleKeyReleasing during tick
    // 4. trigger killNote on reset
    // 4. view-only reference to midi data
    friend class MidiIO ; 
    MidiIO* io_ ;

    std::vector<std::unique_ptr<MidiEventQueue>> queues_ ;
    
    void evaluateReleasedKey(MidiNote& note, size_t ch = 0);
    void killNote(MidiNote& note, size_t ch = 0);
    bool pop(MidiNote& note, size_t ch = 0);
    
};

#endif // MIDI_PROCESSOR_HPP_