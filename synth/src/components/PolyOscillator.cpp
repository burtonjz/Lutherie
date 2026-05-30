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

#include "components/PolyOscillator.hpp"
#include "core/BaseComponent.hpp"
#include "types/ComponentType.hpp"
#include "types/Waveform.hpp"
#include "params/ParameterMap.hpp"
#include "params/ModulationParameter.hpp"
#include "types/ParameterType.hpp"
#include "core/ModulatorComponent.hpp"
#include "config/Config.hpp"

#include <cmath>
#include <utility>
#include <spdlog/spdlog.h>

PolyOscillator::PolyOscillator(ComponentId id, PolyOscillatorConfig cfg):
    BaseComponent(id, ComponentType::PolyOscillator),
    AudioSignalComponent(0,1),
    MidiEventListener(),
    children_(),
    modulators_{},
    modulationData_({})
{
    parameters_->add<ParameterType::WAVEFORM>(cfg.waveform,false);
    parameters_->add<ParameterType::GAIN>(1.0 , false);
    parameters_->add<ParameterType::DETUNE>(0, false);

    updateGain();

    childPool_.initializeAll(*parameters_, 0.0);
}

bool PolyOscillator::isGenerative() const {
    return true ;
}

void PolyOscillator::calculateSample(){
    double v = 0.0 ;
    childPool_.forEachActive([&](Oscillator& obj, std::size_t index){
        obj.calculateSample();
        v += obj.data()[index];
    }, bufferIndex_);
    setBufferValue(0, v);
}

void PolyOscillator::tick(){
    AudioSignalComponent::tick();
    childPool_.forEachActive([&](Oscillator& obj){
        obj.tick();
    });
}

void PolyOscillator::clearBuffer(){
    AudioSignalComponent::clearBuffer();
    // now clear children
    childPool_.forEachActive([](Oscillator& obj){
        obj.clearBuffer();
    });
}

void PolyOscillator::onKeyPressed(const ActiveNote* anote, bool rePress){
    auto it = children_.find(anote->note.getMidiNote());
    if ( rePress && it != children_.end() ){
        Oscillator* osc = it->second ;
        if ( osc ){
            osc->setFrequency(anote->note.getFrequency());
            osc->setAmplitude(anote->note.getMidiVelocity() / 127.0 );
            updateModulationInitialValue(osc);
        }
        return ;
    }

    if ( Oscillator* osc = childPool_.allocate() ){
        osc->setBufferIndex(bufferIndex_); // sync up buffers
        osc->addReferenceParameters(*parameters_);
        osc->setFrequency(anote->note.getFrequency());
        osc->setAmplitude(anote->note.getMidiVelocity() / 127.0 );
        setOverrides(osc, anote->note.getMidiNote());
        
        children_.insert(std::make_pair(anote->note.getMidiNote(),osc));
    } 
}

void PolyOscillator::onKeyReleased(ActiveNote anote){
    auto it = children_.find(anote.note.getMidiNote());
    if ( it != children_.end() ){
        updateModulationInitialValue(it->second);
    }
}

void PolyOscillator::onKeyOff(ActiveNote anote){
    auto it = children_.find(anote.note.getMidiNote());
    if ( it != children_.end() ){
        childPool_.release(it->second);
        children_.erase(it);
    }
}

ModulatorComponent* PolyOscillator::getParameterModulator(ParameterType p) const {
    return modulators_.at(static_cast<size_t>(p)) ;
}

ModulatorComponent* PolyOscillator::getParameterDepthModulator(ParameterType p) const {
    return depthModulators_.at(static_cast<size_t>(p)) ;
}

double PolyOscillator::getParameterDepth(ParameterType p) const {
    auto parentParam = parameters_->getParameter(p);
    if ( parentParam ){
        return BaseComponent::getParameterDepth(p);
    }

    auto d = depthOverrides_[static_cast<int>(p)];

    if ( !d.has_value() ){
        return GET_PARAMETER_TRAIT_MEMBER(ParameterType::DEPTH, defaultValue);
    }

    return d.value() ;
}

void PolyOscillator::setParameterDepth(ParameterType p, double depth){
    auto parentParam = parameters_->getParameter(p);
    if ( parentParam ){
        BaseComponent::setParameterDepth(p, depth );
        return ;
    }

    depthOverrides_[static_cast<int>(p)] = depth ;
    childPool_.forEachActive(&Oscillator::setParameterDepth, p, depth);
}

ModulationStrategy PolyOscillator::getParameterModulationStrategy(ParameterType p) const {
    auto parentParam = parameters_->getParameter(p);
    if ( parentParam ){
        return BaseComponent::getParameterModulationStrategy(p);
    }

    auto s = strategyOverrides_[static_cast<int>(p)];

    if ( ! s.has_value() ){
        return GET_PARAMETER_MODULATION_STRATEGY(p, ModulatorRange::UNKNOWN);
    }
    
    return s.value() ;
}

void PolyOscillator::setParameterModulationStrategy(ParameterType p, ModulationStrategy strat){
    auto parentParam = parameters_->getParameter(p);
    if ( parentParam ){
        BaseComponent::setParameterModulationStrategy(p, strat);
        return ;
    }

    strategyOverrides_[static_cast<int>(p)] = strat ;
    childPool_.forEachActive(&Oscillator::setParameterModulationStrategy, p, strat);
}

void PolyOscillator::updateParameters(){
    BaseComponent::updateParameters();
    childPool_.forEachActive(&Oscillator::updateParameters);
}

void PolyOscillator::onSetParameterModulation(ParameterType p, ModulatorComponent* m, ModulationData d){
    if ( d.isEmpty() ){
        auto required = m->getRequiredModulationParameters();
        for ( auto mp : required ){
            d.set(mp, 0.0f);
        }
    }
    modulators_[static_cast<size_t>(p)] = m ;
    modulationData_[static_cast<size_t>(p)] = d ;
    strategyOverrides_[static_cast<size_t>(p)] = GET_PARAMETER_MODULATION_STRATEGY(p, m->getModulatorRange());

    childPool_.forEachActive(&Oscillator::setParameterModulation, p, m, d);
}

void PolyOscillator::onRemoveParameterModulation(ParameterType p){
    modulators_[static_cast<size_t>(p)] = nullptr ;
    modulationData_[static_cast<size_t>(p)] = {} ;

    childPool_.forEachActive(&Oscillator::removeParameterModulation, p);
}

void PolyOscillator::onSetParameterDepthModulation(ParameterType p, ModulatorComponent* m, ModulationData d){
    if ( d.isEmpty() ){
        auto required = m->getRequiredModulationParameters();
        for ( auto mp : required ){
            d.set(mp, 0.0f);
        }
    }
    depthModulators_[static_cast<size_t>(p)] = m ;
    depthModulationData_[static_cast<size_t>(p)] = d ;

    childPool_.forEachActive(&Oscillator::setParameterDepthModulation, p, m, d);
}

void PolyOscillator::onRemoveParameterDepthModulation(ParameterType p){
    depthModulators_[static_cast<size_t>(p)] = nullptr ;
    depthModulationData_[static_cast<size_t>(p)] = {} ;

    childPool_.forEachActive(&Oscillator::removeParameterDepthModulation, p);
}

void PolyOscillator::updateGain(){
    auto s = std::string(Waveform::getWaveforms()[parameters_->getParameter<ParameterType::WAVEFORM>()->getValue()]) ;
    float gain = Config::get<float>("oscillator." + s + ".auto_gain").value() / 
        std::sqrt(Config::get<int>("oscillator.expected_voices").value()) ;
    SPDLOG_DEBUG("setting gain to {}", gain);
    parameters_->getParameter<ParameterType::GAIN>()->setValue(gain);
}

void PolyOscillator::updateModulationInitialValue(Oscillator* osc){
    for ( auto p : osc->getParameters()->getModulatableParameters()){
        if ( modulators_[static_cast<size_t>(p)] ){
            auto d = osc->getParameters()->getParameter(p)->getModulationData();
            if ( 
                d->has(ModulationParameter::INITIAL_VALUE) &&
                d->has(ModulationParameter::LAST_OUTPUT)
            ){
                d->set(ModulationParameter::INITIAL_VALUE, d->get(ModulationParameter::LAST_OUTPUT)) ;
            }
        }
    } 
}

void PolyOscillator::setOverrides(Oscillator* osc, uint8_t midiNote){
    if ( ! osc ) return ;

    // modulation depth & strategy
    for ( const auto& p : osc->getParameters()->getModulatableParameters() ){
        // modulation depth
        int idx = static_cast<int>(p);
        if ( depthOverrides_.at(idx).has_value() ){
            osc->setParameterDepth(p, depthOverrides_.at(idx).value());
        }

        // modulation strategy
        if ( strategyOverrides_.at(idx).has_value() ){
            osc->setParameterModulationStrategy(p, strategyOverrides_.at(idx).value());
        }

        // set modulator & update modulation data
        auto pIndex = static_cast<size_t>(p);
        if ( modulators_[pIndex] ){
            auto modParams = modulators_[pIndex]->getRequiredModulationParameters();
            if ( modParams.contains(ModulationParameter::MIDI_NOTE)){
                modulationData_[static_cast<size_t>(p)].set(ModulationParameter::MIDI_NOTE, midiNote);
            }
            osc->getParameters()->getParameter(p)
                ->setModulation(modulators_[pIndex], modulationData_[pIndex]);
        }   

        // set depth modulator & update data
        if ( depthModulators_[pIndex] ){
            auto modParams = depthModulators_[pIndex]->getRequiredModulationParameters();
            if ( modParams.contains(ModulationParameter::MIDI_NOTE)){
                depthModulationData_[static_cast<size_t>(p)].set(ModulationParameter::MIDI_NOTE, midiNote);
            }
            osc->getParameters()->getParameter(p)->getDepth()
                ->setModulation(depthModulators_[pIndex], depthModulationData_[pIndex]);
        }   
    }
}