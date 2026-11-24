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

#ifndef __MODULATOR_LINEAR_FADER_HPP_
#define __MODULATOR_LINEAR_FADER_HPP_

#include "core/Node.hpp"
#include "interfaces/ComponentInfo.hpp"
#include "interfaces/MidiProcessor.hpp"
#include "interfaces/ModulationSource.hpp"
#include "interfaces/ParameterAccessor.hpp"
#include "configs/LinearFaderConfig.hpp"

class LinearFader : 
    public Node,
    public ComponentInfo,
    public ModulationSource,
    public MidiProcessor 
{
private:
    SlotAccessor<ParameterType::ATTACK> attack_ ;
    SlotAccessor<ParameterType::RELEASE> release_ ;
     
public:
    const static ComponentType staticType ;

    LinearFader(int id, LinearFaderConfig cfg);

    LinearFader(const LinearFader&) = delete ;
    LinearFader& operator=(const LinearFader&) = delete ;
    LinearFader(LinearFader&&) = delete ;
    LinearFader& operator=(LinearFader&&) = delete ;
    
    // MODULATION SOURCE
    double modulate(double value, ModulationData* mData) const override ;

    // MIDI PROCESSOR
    virtual bool shouldKillNote(const MidiNote& note, size_t ch = 0) const override ;

    double handleAttack(float startLevel, float time) const ;
    double handleRelease(float startLevel, float time) const ;
};

#endif // __MODULATOR_LINEAR_FADER_HPP_