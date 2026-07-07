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

#include "components/Sequencer.hpp"
#include "types/ComponentType.hpp"
#include "types/ParameterType.hpp"
#include "params/ParameterMap.hpp"
#include "params/ParameterCollection.hpp"

#include <spdlog/spdlog.h>

Sequencer::Sequencer(ComponentId id, SequencerConfig cfg):
    BaseComponent(id, ComponentType::Sequencer),
    currentTime_(0),
    lastQueriedBeat_(-1),
    lastStatus_(true)
{
    parameters_->add<ParameterType::STATUS>(true, false);
    parameters_->add<ParameterType::BPM>(cfg.bpm, false);
    parameters_->add<ParameterType::DURATION>(cfg.length, false, 0, cfg.max_length);

    parameters_->addCollection<ParameterType::MIDI_VALUE>({});
    parameters_->addCollection<ParameterType::VELOCITY>({});
    parameters_->addCollection<ParameterType::START_POSITION>({});
    parameters_->addCollection<ParameterType::DURATION>({});

    parameters_->getParameter(ParameterType::BPM)->addListener(this);
    parameters_->getParameter(ParameterType::DURATION)->addListener(this);
}

void Sequencer::onTick(float dt){
    if ( !parameters_->getParameter<ParameterType::STATUS>()->getValue() ){
        if ( lastStatus_ ){
            SPDLOG_DEBUG("Status switched off. Resetting sequencer");
            reset();
        }
        lastStatus_ = false ;
        return ;
    }
    lastStatus_ = true ;

    int bpm = parameters_->getParameter<ParameterType::BPM>()->getValue() ;
    float maxBeats = parameters_->getParameter<ParameterType::DURATION>()->getValue();
    float currentBeat = std::fmod((currentTime_ * bpm ) / 60.0f , maxBeats);

    // how many beats have passed since last query?
    float beatsSinceLastQuery ;
    if ( currentBeat < lastQueriedBeat_ ){
        beatsSinceLastQuery = (maxBeats - lastQueriedBeat_) + currentBeat ;
    } else {
        beatsSinceLastQuery = currentBeat - lastQueriedBeat_ ;
    }

    // check to see if sufficient time has passed to process events again
    auto durations = parameters_->getCollection<ParameterType::DURATION>();
    if ( beatsSinceLastQuery < durations->getMinValue() ) return ;

    // Process Events...
    auto notes = parameters_->getCollection<ParameterType::MIDI_VALUE>();
    auto velocities = parameters_->getCollection<ParameterType::VELOCITY>();
    auto starts = parameters_->getCollection<ParameterType::START_POSITION>();

    for ( int i : notes->getIndices() ){
        float start = starts->getValue(i);
        float end = start + durations->getValue(i);
        
        if ( end > maxBeats ) end = maxBeats ;
        if ( std::abs(start - end) < 0.001 ) continue ; // skip notes near 0 length

        
        if ( currentBeat < lastQueriedBeat_ ){ // handle loop around
            // Case 1: last query to loop end
            if ( start > lastQueriedBeat_ && start <= maxBeats ){
                pushToQueue(notes->getValue(i), velocities->getValue(i), true);
                continue ;
            }
            if ( end > lastQueriedBeat_ && end <= maxBeats ){
                pushToQueue(notes->getValue(i), velocities->getValue(i), false);
                continue ;
            }

            // Case 2: loop start to current beat
            if ( start >= 0.0f && start <= currentBeat ){
                pushToQueue(notes->getValue(i), velocities->getValue(i), true);
                continue ;
            }
            if ( end >= 0.0f && end <= currentBeat ){
                pushToQueue(notes->getValue(i), velocities->getValue(i), false);
                continue ;
            }
        } else { // hasn't looped yet
            if ( start > lastQueriedBeat_ && start <= currentBeat ){
                pushToQueue(notes->getValue(i), velocities->getValue(i), true);
                continue ;
            }
            if ( end > lastQueriedBeat_ && end <= currentBeat ){
                pushToQueue(notes->getValue(i), velocities->getValue(i), false);
                continue ;
            }
        }
    }

    lastQueriedBeat_ = currentBeat ;
    currentTime_ += dt ;
}

void Sequencer::pushToQueue(uint8_t midiNote, uint8_t velocity, bool noteOn){
    SPDLOG_DEBUG("sending note to queue (note={},velocity={},on={})", midiNote, velocity, noteOn);
    MidiNote m = MidiNote(midiNote, velocity, noteOn);
    ActiveNote anote = {m, 0};
    if ( noteOn ){
        queue_.push({MidiEvent::Type::NotePressed, anote});
    } else {
        queue_.push({MidiEvent::Type::NoteReleased, anote});
    }
}

void Sequencer::onReset(){
    currentTime_ = 0 ;
    lastQueriedBeat_ = -1 ;
}

void Sequencer::onParameterChanged([[maybe_unused]] ParameterType p,[[maybe_unused]] bool isCollection){
    if ( !isCollection ){
        reset();
        return ;
    }    
}