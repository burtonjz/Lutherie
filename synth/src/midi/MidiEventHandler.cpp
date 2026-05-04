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

#include "MidiEventHandler.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

void MidiEventHandler::notifyKeyPressed(const ActiveNote* note, bool rePressed) {
    for ( auto* li : listeners_ ){
        if (auto h = dynamic_cast<MidiEventHandler*>(li)){
            h->handleKeyPressed(note->note);
        } else {
            li->onKeyPressed(note, rePressed);
        }
    }
}

// notify downstream listeners of a key release event (only if not handler)
void MidiEventHandler::notifyKeyReleased(const ActiveNote& note) {
    for ( auto* li : listeners_ ){
        // only notify end node listeners of release events
        if ( !dynamic_cast<MidiEventHandler*>(li) ){
            li->onKeyReleased(note);
        }
    } 
}

// notify downstream listeners of a key off event (release if handler)
void MidiEventHandler::notifyKeyOff(const ActiveNote& note) {
    for ( auto* li : listeners_ ){
        if ( auto h = dynamic_cast<MidiEventHandler*>(li) ){
            h->handleKeyReleased(note.note);
        } else {
            li->onKeyOff(note);
        }
    } 
}

void MidiEventHandler::notifyPitchbend(uint16_t pitchbend) {
    for ( auto* li : listeners_ ) li->onPitchbend(pitchbend);
}

MidiEventHandler::MidiEventHandler():
    BaseComponent(),
    notes_(),
    noteIndices_()
{}

void MidiEventHandler::addListener(MidiEventListener* listener){
    if ( std::find(listeners_.begin(), listeners_.end(), listener) != listeners_.end() ) return ;
    listeners_.push_back(listener);
    listener->addHandler(this);
}

void MidiEventHandler::removeListener(MidiEventListener* listener){
    auto it = std::find(listeners_.begin(), listeners_.end(), listener);
    if ( it != listeners_.end() ){
        listener->removeHandler(this);
        listeners_.erase(it);
    }
}

std::vector<MidiEventListener*>& MidiEventHandler::getListeners(){
    return listeners_ ;
}

bool MidiEventHandler::isNoteActive(uint8_t n) const {
    for ( uint8_t i = 0 ; i < activeCount_; ++i ){
        if (noteIndices_[i] == n ) return true ;
    }
    return false ;
}

void MidiEventHandler::activateNote(const ActiveNote& anote){
    uint8_t midiNote = anote.note.getMidiNote() ;
    notes_[midiNote] = anote ;
    lastPressedNote_ = midiNote ;

    if ( isNoteActive(midiNote) ){
        return ;
    }
    noteIndices_[activeCount_++] = midiNote ;
}

void MidiEventHandler::deactivateNote(uint8_t n){
    lastReleasedNote_ = n ;
    for ( uint8_t i = 0 ; i < activeCount_; ++i ){
        if ( noteIndices_[i] == n ){
            noteIndices_[i] = noteIndices_[--activeCount_];
            break ;
        }
    }
}

void MidiEventHandler::onKeyPressed(const ActiveNote* note, bool rePressed ){
    MidiEvent e = { MidiEvent::Type::NotePressed, *note, rePressed };
    queue_.push(e);
}

void MidiEventHandler::onKeyReleased(ActiveNote anote){
    anote.resetTime();
    MidiEvent e = { MidiEvent::Type::NoteReleased, anote };
    queue_.push(e);
}

void MidiEventHandler::onKeyOff(ActiveNote anote){
    MidiEvent e = { MidiEvent::Type::NoteOff, anote };
    queue_.push(e);
}

void MidiEventHandler::onPitchbend(uint16_t pitchbend){
    notifyPitchbend(pitchbend);
}

// handler functions for root-level midi input from midi state
void MidiEventHandler::handleKeyPressed(const MidiNote note){
    ActiveNote n{note};

    bool rePress = false ;
    if ( isNoteActive(note.getMidiNote()) ){
        rePress = true ;
    }
    onKeyPressed(&n, rePress);
};

void MidiEventHandler::handleKeyReleased(const MidiNote note){
    auto anote = notes_[note.getMidiNote()] ;
    if ( ! isNoteActive(note.getMidiNote()) ){
        SPDLOG_WARN("Received release event for midi note {}, but that note is not currently active."
            "This may be intentionally caused by a child class implementation.", note.getMidiNote());
    }
    anote.note = note ;
    onKeyReleased(anote);
};

void MidiEventHandler::handlePitchbend(uint16_t pitchbend){
    onPitchbend(pitchbend);
};

bool MidiEventHandler::shouldKillNote(const ActiveNote& anote) const {
    return !anote.note.getStatus() ;
}

void MidiEventHandler::processEvents(){
    MidiEvent e ;
    while (queue_.pop(e)){    
        ActiveNote anote = e.anote ;
        uint8_t midiNote = anote.note.getMidiNote() ;
        switch (e.type){
        case MidiEvent::Type::NotePressed:
            activateNote(anote);
            notifyKeyPressed(&notes_[midiNote], e.rePressed);
            break ;
        case MidiEvent::Type::NoteReleased:
            notes_[e.anote.note.getMidiNote()] = anote ;
            notifyKeyReleased(anote);
            break ;
        case MidiEvent::Type::NoteOff:
            notifyKeyOff(anote);
            deactivateNote(midiNote);
            break ;
        }
    }
}

void MidiEventHandler::tick(float dt){
    processEvents();

    onTick(dt); 

    for ( uint8_t i = 0; i < activeCount_ ; ++i ){
        ActiveNote& note = notes_[noteIndices_[i]];
        if ( shouldKillNote(note) ){
            MidiEvent e = {MidiEvent::Type::NoteOff, note};
            queue_.push(e);
        } else {
            note.updateTime(dt);
        }
    }
}

void MidiEventHandler::onTick([[maybe_unused]] float dt){
}

void MidiEventHandler::reset(){
    processEvents();

    onReset(); 
    for ( uint8_t i = 0 ; i < activeCount_; ++i ){
        ActiveNote& note = notes_[noteIndices_[i]];
        note.note.setStatus(false);
        queue_.push({MidiEvent::Type::NoteOff, note});
        SPDLOG_DEBUG(
            "sending note off to queue (note={})", 
            static_cast<int>(note.note.getMidiNote()), note.note.getStatus()
        );
    }

    processEvents();
}

void MidiEventHandler::onReset(){
}