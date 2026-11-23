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

#include "interfaces/MidiProcessor.hpp"

void MidiProcessor::handleKeyPressed(MidiNote note, size_t ch){
    push(std::move(note), ch);
}

void MidiProcessor::handleKeyReleased(MidiNote note, size_t ch){
    push(std::move(note), ch);
}

bool MidiProcessor::shouldKillNote(const MidiNote&, size_t) const {
    return true ;
}

void MidiProcessor::onTick(float){}
void MidiProcessor::onReset(){}

bool MidiProcessor::push(MidiNote note, size_t ch){
    return queues_[ch]->push(std::move(note));
}

void MidiProcessor::evaluateReleasedKey(MidiNote& note, size_t ch){
    if ( shouldKillNote(note, ch) ) killNote(note, ch);
}

void MidiProcessor::killNote(MidiNote& note, size_t ch){
    note.setStatus(NoteStatus::KILLING);
        
    MidiNote n = note ;
    n.setStatus(NoteStatus::KILLING);
    push(std::move(n), ch);
}

bool MidiProcessor::pop(MidiNote& note, size_t ch){
    return queues_[ch]->pop(note);
}