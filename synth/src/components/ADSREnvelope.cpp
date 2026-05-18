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

#include "components/ADSREnvelope.hpp"
#include "params/ModulationParameter.hpp"
#include "params/ParameterMap.hpp"
#include "types/ParameterType.hpp"
ADSREnvelope::ADSREnvelope(ComponentId id, ADSREnvelopeConfig cfg):
    BaseComponent(id,ComponentType::ADSREnvelope),
    ModulatorComponent()
{
    parameters_->add<ParameterType::ATTACK>(cfg.attack, true);
    parameters_->add<ParameterType::DECAY>(cfg.decay, true);
    parameters_->add<ParameterType::SUSTAIN>(cfg.sustain, true);
    parameters_->add<ParameterType::RELEASE>(cfg.release, true);
    parameters_->add<ParameterType::TRIGGER>(cfg.trigger, false);

    requiredParams_ = {
        ModulationParameter::MIDI_NOTE, 
        ModulationParameter::INITIAL_VALUE
    };
}

double ADSREnvelope::modulate([[maybe_unused]] double value, ModulationData* mData) const {
    double output = 0.0 ;

    // check required data
    if ( !mData ) return output ; 

    if ( !mData->has(ModulationParameter::INITIAL_VALUE) ){
        mData->set(ModulationParameter::INITIAL_VALUE, 0.0f);
    }
    if ( !mData->has(ModulationParameter::LAST_OUTPUT) ){ 
        mData->set(ModulationParameter::LAST_OUTPUT, 0.0f) ;    
    }
    
    // get the midi note
    bool monophonicMode = !mData->has(ModulationParameter::MIDI_NOTE);
    auto behavior = MonophonicTriggerBehavior::from_uint8(
        parameters_->getParameter<ParameterType::TRIGGER>()->getValue()
    );
    uint8_t midiNote ;
    if ( monophonicMode ){ 
        midiNote = activeCount_ > 0 ? lastPressedNote_ : lastReleasedNote_ ;
        if ( midiNote == 255 ) return output ; // no notes ever played!
        midiNote = resolveMonophonicNote(midiNote, mData, behavior);
        
    } else {
        midiNote = static_cast<uint8_t>(mData->get(ModulationParameter::MIDI_NOTE)) ;
    }
    
    auto anote = notes_[midiNote] ;
    if ( !isNoteActive(midiNote) ) return output ;

    // set start level
    if ( monophonicMode ){
        updateMonophonicState(midiNote, anote.note.getStatus(), mData, behavior);
    }
    float start_level = mData->get(ModulationParameter::INITIAL_VALUE);

    // calculate ADSR
    if ( anote.note.getStatus() ){ // Attack Decay Sustain
        float attack = parameters_->getParameter<ParameterType::ATTACK>()->getInstantaneousValue() ;
        float decay = parameters_->getParameter<ParameterType::DECAY>()->getInstantaneousValue() ;
        float sustain = parameters_->getParameter<ParameterType::SUSTAIN>()->getInstantaneousValue() ;
        
        if ( anote.time <= attack ) {
            output = start_level + ( 1.0f - start_level ) * (anote.time / attack) ;
        } else if ( anote.time <= (attack + decay) ) {
            output = 1.0f - (1.0f - sustain) * ((anote.time - attack) / decay );
        } else {
            output = sustain ;
        }
    } else { // Release
        float release = parameters_->getParameter<ParameterType::RELEASE>()->getInstantaneousValue() ;
        if ( anote.time >= release ){
            output = 0.0 ;
        } else {
            output = start_level * (1 - (anote.time / release)) ;
        }
    }

    mData->set(ModulationParameter::LAST_OUTPUT, output) ;
    return output ;
}

bool ADSREnvelope::shouldKillNote(const ActiveNote& note) const {
    float release = parameters_->getParameter<ParameterType::RELEASE>()->getInstantaneousValue() ;
    return ( !note.note.getStatus() && note.time > release ) ;
}

uint8_t ADSREnvelope::resolveMonophonicNote(
    uint8_t currentNote, ModulationData* mData, 
    MonophonicTriggerBehavior behavior) const 
{
    if ( behavior == MonophonicTriggerBehavior::LEGATO ){
        if ( mData->has(ModulationParameter::LAST_MIDI_NOTE) ){
            uint8_t latchedNote = static_cast<uint8_t>(mData->get(ModulationParameter::LAST_MIDI_NOTE));
            if ( isNoteActive(latchedNote) ){
                return latchedNote ;
            }
        }
    }
    return currentNote ;
}

void ADSREnvelope::updateMonophonicState(
    uint8_t midiNote, bool isPressed, 
    ModulationData* mData, MonophonicTriggerBehavior behavior) const 
{
    bool wasPressed = mData->has(ModulationParameter::LAST_STATE) &&
        mData->get(ModulationParameter::LAST_STATE) > 0.0f ;
    bool noteChanged = mData->has(ModulationParameter::LAST_MIDI_NOTE) &&
        mData->get(ModulationParameter::LAST_MIDI_NOTE) != midiNote ;
    switch ( behavior ){
    case MonophonicTriggerBehavior::LEGATO:
        if ( isPressed != wasPressed ){
            mData->set(ModulationParameter::INITIAL_VALUE, mData->get(ModulationParameter::LAST_OUTPUT));
        }
        break ;
    case MonophonicTriggerBehavior::RETRIGGER_LEGATO:
        if (( isPressed != wasPressed ) || noteChanged ){
            mData->set(ModulationParameter::INITIAL_VALUE, mData->get(ModulationParameter::LAST_OUTPUT));
        }
        break ;
    case MonophonicTriggerBehavior::RETRIGGER_RESET:
        if (( isPressed && !wasPressed ) || noteChanged ){
            mData->set(ModulationParameter::INITIAL_VALUE, 0.0f);
        } else if ( !isPressed && wasPressed ){
            mData->set(ModulationParameter::INITIAL_VALUE, mData->get(ModulationParameter::LAST_OUTPUT));
        }
        break ;
    default:
        break ;
    }
    mData->set(ModulationParameter::LAST_MIDI_NOTE, midiNote);
    mData->set(ModulationParameter::LAST_STATE, isPressed ? 1.0f : 0.0f);
}