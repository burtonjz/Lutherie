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

#include "capabilities/MidiIO.hpp"
#include "interfaces/MidiObserver.hpp"
#include "interfaces/MidiProcessor.hpp"
#include "midi/MidiNote.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>


MidiIO::MidiIO(MidiProcessor& processor, size_t inputs, size_t outputs):
    processor_(processor),
    numInputs_(inputs),
    numOutputs_(outputs),
    notes_(numOutputs_),
    active_(numOutputs_),
    outputEdges_(numOutputs_),
    observers_(numOutputs_),
    lastNotePressed_(MidiNote::INVALID, numOutputs_),
    lastNoteReleased_(MidiNote::INVALID, numOutputs_)
{
    assert(outputs > 1);

    // friend: help construct processor
    processor_.io_ = this ;
    for ( size_t i = 0 ; i < numOutputs_ ; i++ ){
        processor.queues_.push_back(std::make_unique<MidiEventQueue>());
    }
}

size_t MidiIO::inputs() const { 
    return numInputs_ ; 
}

size_t MidiIO::outputs() const { 
    return numOutputs_ ; 
}

void MidiIO::addOutputEdge(MidiIO* edge, size_t edgeChannel, size_t toChannel){
    outputEdges_.at(toChannel).insert({edge, edgeChannel});
}

void MidiIO::removeOutputEdge(MidiIO* edge, size_t edgeChannel, size_t toChannel){
    outputEdges_.at(toChannel).erase({edge, edgeChannel});
}

void MidiIO::addObserver(MidiObserver* obs, size_t ch){
    assert ( ch < numInputs_ );
    auto& observers = observers_[ch];
    auto it = std::find(observers.begin(), observers.end(), obs);
    if ( it != observers.end() ) return ;
    observers.push_back(obs);
    obs->numConnections_++ ;
}

void MidiIO::removeObserver(MidiObserver* obs, size_t ch){
    assert ( ch < numInputs_ );
    auto& observers = observers_[ch];
    auto it = std::find(observers.begin(), observers.end(), obs);
    if ( it == observers.end() ) return ;
    observers.erase(it);
    obs->numConnections_-- ;
}

std::span<MidiObserver* const> MidiIO::observers(size_t ch) const {
    assert( ch < numInputs_ );
    return observers_[ch] ;
}

void MidiIO::setActive(uint8_t note, size_t ch){
    assert( note < 128 );
    assert( ch < numInputs_ );

    const auto& n = notes_[ch][note];
    switch( n.status() ){
    case NoteStatus::PRESSED:
    case NoteStatus::RELEASED:
    case NoteStatus::REPRESSED:
        active_[ch].set(note);
    case NoteStatus::OFF:
    case NoteStatus::KILLING:
        active_[ch].set(note, false);
    }
}

void MidiIO::handleKeyPressed(MidiNote note, size_t ch){
    note.resetTime();
    if ( active_[ch].test(note.note()) ){
        note.setStatus(NoteStatus::REPRESSED);
    }
    processor_.handleKeyPressed(std::move(note), ch);
};

void MidiIO::handleKeyReleased(MidiNote note, size_t ch){
    note.resetTime();
    processor_.handleKeyReleased(std::move(note), ch);
};

void MidiIO::handlePitchbend(float pitchbend, size_t ch){
    // just a simple observe + pass
    for ( auto* obs : observers_[ch] ){
        obs->onPitchbend(pitchbend);
    }

    for ( auto& conn : outputEdges_[ch] ){
        conn.midi->handlePitchbend(pitchbend);
    }
};

void MidiIO::processEvents(){
    MidiNote note ;
    for ( size_t ch = 0 ; ch < numOutputs_ ; ++ch ){
        while ( processor_.queues_[ch]->pop(note) ){
            processEvent(std::move(note), ch);
        }
    }
}

void MidiIO::processEvent(MidiNote note, size_t ch){
    uint8_t v = note.note();
    NoteMap& notes = notes_[ch];

    notes[v] = std::move(note);
    setActive(v, ch);

    switch ( notes[v].status() ){
    case NoteStatus::PRESSED:
    case NoteStatus::REPRESSED:
        lastNotePressed_[ch] = v ;
        break ;
    case NoteStatus::RELEASED:
        lastNoteReleased_[ch] = v ;
        break ;
    default:
        break ;
    }

    // notify observers
    for ( auto* obs : observers_[ch] ){
        obs->onKeyEvent(notes[v]);
    }

    // notify downstream
    // OFF -> RELEASE to behave same as raw midi
    for ( auto& conn : outputEdges_[ch] ){
        switch(notes[v].status()){
        case NoteStatus::PRESSED:
        case NoteStatus::REPRESSED:
            conn.midi->handleKeyPressed(note, conn.channel);
            break ;
        case NoteStatus::OFF:
            conn.midi->handleKeyReleased(note, conn.channel);
            break ;
        default:
            break ;
        }
    }
}

void MidiIO::tick(float dt){
    processor_.onTick(dt); 

    for ( size_t ch = 0; ch < numOutputs_; ++ch ){
        active_[ch].forEach([&](size_t n){
            auto& note = notes_[ch][n];

            if ( note.status() == NoteStatus::KILLING ){
                return ;
            }

            if ( note.status() == NoteStatus::RELEASED ){
                processor_.evaluateReleasedKey(note);
            }
            
            note.updateTime(dt);
        });
    }
}

void MidiIO::reset(){
    processEvents();

    processor_.onReset(); 
    for ( size_t ch = 0; ch < numOutputs_; ++ch ){
        lastNotePressed_[ch]  = MidiNote::INVALID ;
        lastNoteReleased_[ch] = MidiNote::INVALID ;

        active_[ch].forEach([&](size_t n){
            MidiNote& note = notes_[ch][n];
            processor_.killNote(note, ch);
            SPDLOG_DEBUG(
                "killing note: note={}", 
                static_cast<int>(n)
            );
        });
    }

    processEvents();
}

