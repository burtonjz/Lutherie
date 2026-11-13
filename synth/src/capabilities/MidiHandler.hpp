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

#include "interfaces/Capability.hpp"
#include "interfaces/MidiObserver.hpp"
#include "midi/MidiNote.hpp"
#include "midi/MidiEventQueue.hpp"

#include <vector>

class MidiHandler : public Capability, public MidiObserver {
protected:
    std::array<ActiveNote,128> notes_ ;
    std::array<uint8_t,128> noteIndices_ ;
    uint8_t activeCount_ = 0 ;
    
    uint8_t lastPressedNote_ = 255 ;
    uint8_t lastReleasedNote_ = 255 ;
    
    std::vector<MidiObserver*> observers_ ;
    MidiEventQueue queue_ ;
    
public:
    MidiHandler();
    virtual ~MidiHandler() = default ;

    void addObserver(MidiObserver* listener);
    void removeObserver(MidiObserver* listener);
    std::vector<MidiObserver*>& observers();
    
    bool isNoteActive(uint8_t n) const ;
    void activateNote(const ActiveNote& anote);
    void deactivateNote(uint8_t n);
        
    // handler functions for root-level midi input from midi state
    void handleKeyPressed(const MidiNote note);
    void handleKeyReleased(const MidiNote note);
    void handlePitchbend(uint16_t pitchbend);

    // determines if a note should be killed on tick. Default behavior is to kill a note as soon as note off is received via Midi
    virtual bool shouldKillNote(const ActiveNote& anote) const ;
    
    void processEvents();
    void tick(float dt);

    virtual void onTick([[maybe_unused]] float dt); // for adding custom tick actions to child objects

    // responding to connection events
    virtual void onObserverAdded();
    virtual void onObserverRemoved();

    // override observer logic to provide specialized operations
    // when cascading events downstream
    virtual void onKeyPressed(const ActiveNote* anote, bool rePress = false) override ;
    virtual void onKeyReleased(ActiveNote anote) override ;
    virtual void onKeyOff(ActiveNote anote) override ;
    virtual void onPitchbend(uint16_t pitchbend) override ;

protected:
    void reset();
    virtual void onReset();

    /**
     * MidiHandler is also an observer, so that events can be broadcasted
     * in a chain, with the intention that any handler receives events as
     * 
     * Raw MIDI events ON/OFF are mapped to PRESSED/RELEASED, so to accomplish
     * this, Handler Observers receive OFF as RELEASE.

     * Importantly, this means that midi handlers that modify the release time should
     * often be at the end of the chain.
    */
    void notifyKeyPressed(const ActiveNote* note, bool rePressed = false);
    void notifyKeyReleased(const ActiveNote& note); 
    void notifyKeyOff(const ActiveNote& note); 
    void notifyPitchbend(uint16_t pitchbend);
};

#endif // MIDI_HANDLER_HPP_