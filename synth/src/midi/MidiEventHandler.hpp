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

#ifndef __MIDI_EVENT_HANDLER_HPP_
#define __MIDI_EVENT_HANDLER_HPP_

#include "core/BaseComponent.hpp"
#include "midi/MidiNote.hpp"
#include "midi/MidiEventListener.hpp"
#include "midi/MidiEventQueue.hpp"

#include <vector>

class MidiEventHandler : public MidiEventListener, public virtual BaseComponent {
protected:
    std::array<ActiveNote,128> notes_ ;
    std::array<uint8_t,128> noteIndices_ ;
    uint8_t activeCount_ = 0 ;
    
    uint8_t lastPressedNote_ = 255 ;
    uint8_t lastReleasedNote_ = 255 ;
    
    std::vector<MidiEventListener*> listeners_ ;
    MidiEventQueue queue_ ;

    /**
     * The below functions broadcast handler events to downstream objects
     * Note: MidiEventHandlers will broadcast to chained handlers such that 
     * each handler is able to act as if it is receiving raw midi. This allows
     * a chained midi event handler to act in isolation in determining when a
     * note should be killed.

     * This means midiEventHandlers receive OFF as RELEASE, where as the end node
     * midiEventListeners are the only ones that actually receive OFF events

     * Importantly, this means that midi handlers that modify the release time should
     * be at the end of the chain, so that other handlers in the chain don't mask the 
     * release time from downstream modules (see Polyphonic Oscillator implementation for 
     * an example into problems that can arise)
    */
    void notifyKeyPressed(const ActiveNote* note, bool rePressed = false);
    void notifyKeyReleased(const ActiveNote& note); // only sends release event if downstream listener is NOT another handler
    void notifyKeyOff(const ActiveNote& note); // sends release event if downstream listener is another handler
    void notifyPitchbend(uint16_t pitchbend);

public:
    MidiEventHandler();
    virtual ~MidiEventHandler() = default ;

    void addListener(MidiEventListener* listener);
    void removeListener(MidiEventListener* listener);
    std::vector<MidiEventListener*>& getListeners();
    
    bool isNoteActive(uint8_t n) const ;
    void activateNote(const ActiveNote& anote);
    void deactivateNote(uint8_t n);

    // all handlers are listeners so that they can chain operations if desired. 
    // The below functions can be overridded to provide specialized operations when cascading events downwards
    void onKeyPressed(const ActiveNote* note, bool rePressed = false) override ;
    void onKeyReleased(ActiveNote anote) override ;
    void onKeyOff(ActiveNote anote) override ;
    void onPitchbend(uint16_t pitchbend) override ;
        
    // handler functions for root-level midi input from midi state
    void handleKeyPressed(const MidiNote note);
    void handleKeyReleased(const MidiNote note);
    void handlePitchbend(uint16_t pitchbend);

    // determines if a note should be killed on tick. Default behavior is to kill a note as soon as note off is received via Midi
    virtual bool shouldKillNote(const ActiveNote& anote) const ;
    
    void processEvents();
    void tick(float dt);
    virtual void onTick([[maybe_unused]] float dt); // for adding custom tick actions to child objects

protected:
    void reset();
    virtual void onReset();
};

#endif // __MIDI_EVENT_HANDLER_HPP_