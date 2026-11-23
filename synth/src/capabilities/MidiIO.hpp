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

#ifndef MIDI_HANDLER_HPP_
#define MIDI_HANDLER_HPP_

#include "capabilities/Capability.hpp"
#include "containers/Mask.hpp"
#include "midi/MidiNote.hpp"
#include "routing/MidiConnection.hpp"

#include <vector>

// forward declarations
class MidiProcessor ;
class MidiObserver ;

class MidiIO : public Capability {
private:
    static constexpr uint8_t INVALID_NOTE = 255 ;
    using NoteMap = std::array<MidiNote,128> ;

    MidiProcessor& processor_ ;
    
    size_t numInputs_ ;
    size_t numOutputs_ ;

    std::vector<NoteMap> notes_ ; 
    std::vector<Mask<128>> active_ ;
    
    std::vector<MidiConnectionSet> outputEdges_ ;
    std::vector<std::vector<MidiObserver*>> observers_ ;
    

    uint8_t lastPressedNote_ = INVALID_NOTE ;
    uint8_t lastReleasedNote_ = INVALID_NOTE ;
    
public:
    MidiIO(MidiProcessor& processor, size_t inputs, size_t outputs);
    ~MidiIO() = default ;

    size_t inputs() const ;
    size_t outputs() const ;

    void addOutputEdge(MidiIO* edge, size_t edgeChannel, size_t toChannel);
    void removeOutputEdge(MidiIO* edge, size_t edgeChannel, size_t toChannel);

    void addObserver(MidiObserver* obs, size_t ch = 0);
    void removeObserver(MidiObserver* obs, size_t ch = 0);

    std::span<MidiObserver* const> observers(size_t ch = 0) const ;
    
    void handleKeyPressed(MidiNote note, size_t ch = 0);
    void handleKeyReleased(MidiNote note, size_t ch = 0);
    void handlePitchbend(float pitchbend, size_t ch = 0);

    // determines if a note should be killed on tick. 
    // Default behavior is to kill a note as soon as note off is received via Midi
    virtual bool shouldKillNote(const MidiNote& anote) const ;
    
    void processEvents();
    void tick(float dt);

private:
    void reset();

    void setActive(uint8_t note, size_t ch = 0);

    void processEvent(MidiNote note, size_t ch = 0);
    
};

#endif // MIDI_HANDLER_HPP_