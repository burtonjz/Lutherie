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

#include "components/LinearFader.hpp"
#include "core/BaseComponent.hpp"
#include "types/ComponentType.hpp"
#include "params/ModulationParameter.hpp"
#include "params/ParameterMap.hpp"
#include "types/ParameterType.hpp"

const ComponentType LinearFader::staticType = ComponentType::LinearFader ;

LinearFader::LinearFader(ComponentId id, LinearFaderConfig cfg):
    BaseComponent(id, ComponentType::LinearFader),
    BaseModulator()
{
    parameters_->add<ParameterType::ATTACK>(cfg.attack, true);
    parameters_->add<ParameterType::RELEASE>(cfg.release, true);

    requiredParams_ = {
        ModulationParameter::MIDI_NOTE, 
        ModulationParameter::INITIAL_VALUE
    };
}

double LinearFader::modulate([[maybe_unused]] double value, ModulationData* mData) const {
    double output = 0.0 ;

    // check required data / set to defaults
    if ( !mData ) return output ; 
    if ( !mData->has(ModulationParameter::MIDI_NOTE) ) return output ;
    
    if ( !mData->has(ModulationParameter::INITIAL_VALUE) ){ 
        mData->set(ModulationParameter::INITIAL_VALUE, 0.0f) ;     
    }
    if ( !mData->has(ModulationParameter::LAST_OUTPUT) ){ 
        mData->set(ModulationParameter::LAST_OUTPUT, 0.0f );    
    }
    
    uint8_t midiNote = static_cast<uint8_t>(mData->get(ModulationParameter::MIDI_NOTE)) ;
    auto anote = notes_[midiNote] ;
    if ( !isNoteActive(midiNote) ) return output ;

    float start_level = mData->get(ModulationParameter::INITIAL_VALUE) ;
    
    if ( anote.note.getStatus() ){
        // note is pressed
        float attack = parameters_->getParameter<ParameterType::ATTACK>()->getInstantaneousValue() ;
        if ( anote.time <= attack ) {
            output = start_level + (1.0f - start_level) * (anote.time / attack) ;
        } else {
            output = 1.0f ; // full modulation
        }
    } else {
        // note is released
        float release = parameters_->getParameter<ParameterType::RELEASE>()->getInstantaneousValue() ;
        if ( anote.time >= release ){
            output = 0.0 ;
        } else {
            output =  start_level * (1.0f - ( anote.time / release )); 
        }
    }

    mData->set(ModulationParameter::LAST_OUTPUT, output) ;
    return output ;
}

bool LinearFader::shouldKillNote(const ActiveNote& note) const {
    float release = parameters_->getParameter<ParameterType::RELEASE>()->getInstantaneousValue() ;
    return ( !note.note.getStatus() && note.time > release ) ;
}