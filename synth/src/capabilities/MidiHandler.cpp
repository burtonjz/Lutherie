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

#include "MidiHandler.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

void MidiHandler::notifyKeyPressed(const ActiveNote* note, bool rePressed) {
    for ( auto* li : listeners_ ){
        if (auto h = dynamic_cast<MidiHandler*>(li)){
            h->handleKeyPressed(note->note);
        } else {
            li->onKeyPressed(note, rePressed);
        }
    }
}

// notify downstream listeners of a key release event (only if not handler)
void MidiHandler::notifyKeyReleased(const ActiveNote& note) {
    for ( auto* li : listeners_ ){
        // only notify end node listeners of release events
        if ( !dynamic_cast<MidiHandler*>(li) ){
            li->onKeyReleased(note);
        }
    } 
}

// notify downstream listeners of a key off event (release if handler)
void MidiHandler::notifyKeyOff(const ActiveNote& note) {
    for ( auto* li : listeners_ ){
        if ( auto h = dynamic_cast<MidiHandler*>(li) ){
            h->handleKeyReleased(note.note);
        } else {
            li->onKeyOff(note);
        }
    } 
}

void MidiHandler::notifyPitchbend(uint16_t pitchbend) {
    for ( auto* li : listeners_ ) li->onPitchbend(pitchbend);
}

MidiHandler::MidiHandler():
    notes_(),
    noteIndices_()
{}

void MidiHandler::addListener(MidiListener* listener){
    if ( std::find(listeners_.begin(), listeners_.end(), listener) != listeners_.end() ) return ;
    listeners_.push_back(listener);
    listener->addHandler(this);
    onListenerAdded();
}

void MidiHandler::removeListener(MidiListener* listener){
    auto it = std::find(listeners_.begin(), listeners_.end(), listener);
    if ( it != listeners_.end() ){
        listener->removeHandler(this);
        listeners_.erase(it);
        onListenerRemoved();
    }
}

std::vector<MidiListener*>& MidiHandler::getListeners(){
    return listeners_ ;
}

bool MidiHandler::isNoteActive(uint8_t n) const {
    for ( uint8_t i = 0 ; i < activeCount_; ++i ){
        if (noteIndices_[i] == n ){
            return !notes_.at(noteIndices_[i]).pendingOff ;
        } 
    }
    return false ;
}

void MidiHandler::activateNote(const ActiveNote& anote){
    uint8_t midiNote = anote.note.getMidiNote() ;
    notes_[midiNote] = anote ;
    lastPressedNote_ = midiNote ;

    if ( isNoteActive(midiNote) ){
        return ;
    }
    noteIndices_[activeCount_++] = midiNote ;
}

void MidiHandler::deactivateNote(uint8_t n){
    lastReleasedNote_ = n ;
    for ( uint8_t i = 0 ; i < activeCount_; ++i ){
        if ( noteIndices_[i] == n ){
            noteIndices_[i] = noteIndices_[--activeCount_];
            break ;
        }
    }
}

void MidiHandler::onKeyPressed(const ActiveNote* note, bool rePressed ){
    MidiEvent e = { MidiEvent::Type::NotePressed, *note, rePressed };
    queue_.push(e);
}

void MidiHandler::onKeyReleased(ActiveNote anote){
    anote.resetTime();
    MidiEvent e = { MidiEvent::Type::NoteReleased, anote };
    queue_.push(e);
}

void MidiHandler::onKeyOff(ActiveNote anote){
    MidiEvent e = { MidiEvent::Type::NoteOff, anote };
    queue_.push(e);
}

void MidiHandler::onPitchbend(uint16_t pitchbend){
    notifyPitchbend(pitchbend);
}

// handler functions for root-level midi input from midi state
void MidiHandler::handleKeyPressed(const MidiNote note){
    ActiveNote n{note};

    bool rePress = false ;
    if ( isNoteActive(note.getMidiNote()) ){
        rePress = true ;
    }
    onKeyPressed(&n, rePress);
};

void MidiHandler::handleKeyReleased(const MidiNote note){
    auto anote = notes_[note.getMidiNote()] ;
    anote.note = note ;
    onKeyReleased(anote);
};

void MidiHandler::handlePitchbend(uint16_t pitchbend){
    onPitchbend(pitchbend);
};

bool MidiHandler::shouldKillNote(const ActiveNote& anote) const {
    return !anote.note.getStatus() ;
}

void MidiHandler::processEvents(){
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

void MidiHandler::tick(float dt){
    onTick(dt); 

    for ( uint8_t i = 0; i < activeCount_ ; ++i ){
        ActiveNote& note = notes_[noteIndices_[i]];

        if ( note.pendingOff ) continue ;

        if ( shouldKillNote(note) ){
            MidiEvent e = {MidiEvent::Type::NoteOff, note};
            note.pendingOff = true ;
            queue_.push(e);
        } else {
            note.updateTime(dt);
        }
    }
}

void MidiHandler::onTick([[maybe_unused]] float dt){
}

void MidiHandler::reset(){
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

void MidiHandler::onReset(){}
void MidiHandler::onListenerAdded(){}
void MidiHandler::onListenerRemoved(){}