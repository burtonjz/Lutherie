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

#include "components/Oscillator.hpp"
#include "dsp/detune.hpp"
#include "types/ComponentType.hpp"
#include "dsp/Wavetable.hpp"
#include "params/ParameterMap.hpp"
#include "types/ParameterType.hpp"
#include <cmath>

Oscillator::Oscillator(ComponentId id, OscillatorConfig cfg):
    BaseComponent(id, ComponentType::Oscillator),
    AudioSignalComponent(0,1),
    phase_(0),
    increment_(0),
    generator_(std::random_device{}())
{
    Wavetable::generate();
    distribution_ = std::uniform_int_distribution<size_t>(
        0, 
        Wavetable::getWavetable(Waveform::NOISE).second - 1
    );

    parameters_->add<ParameterType::WAVEFORM>(cfg.waveform,false);
    parameters_->add<ParameterType::AMPLITUDE>(1.0,true);
    parameters_->add<ParameterType::FREQUENCY>(cfg.frequency, true, 
        0.0, sampleRate_ * 0.495 ); // limit to nyquist
    parameters_->add<ParameterType::GAIN>(1.0,false);
}


Oscillator::Oscillator(ParameterMap& parent, double frequency):
    BaseComponent(-1, ComponentType::Oscillator),
    AudioSignalComponent(0,1),
    phase_(0),
    increment_(0)
{
    Wavetable::generate();

    parameters_->addReferences(parent);
    parameters_->add<ParameterType::AMPLITUDE>(1.0,true);
    parameters_->add<ParameterType::FREQUENCY>(frequency, true, 
        0.0, sampleRate_ * 0.495 ); // limit to nyquist frequency
}

bool Oscillator::isGenerative() const {
    return true ;
}

void Oscillator::calculateSample(){
    Waveform wf = Waveform::from_uint8(parameters_->getParameter<ParameterType::WAVEFORM>()->getValue()); 
    auto w = Wavetable::getWavetable(wf) ;
    double frac, wavetableIndex, sample ;
    int index_floor ;

    // calculate sample
    if (wf == Waveform::NOISE){
        size_t idx = distribution_(generator_);
        sample = w.first[idx];
    } else {
        wavetableIndex = phase_ * (w.second - 1);
        index_floor = static_cast<int>(wavetableIndex);
        frac = wavetableIndex - index_floor;
        sample = (1.0 - frac) * w.first[index_floor] + frac * w.first[index_floor + 1];
    }
    auto amplitude = parameters_->getParameter<ParameterType::AMPLITUDE>()->getInstantaneousValue();
    auto gain = parameters_->getParameter<ParameterType::GAIN>()->getInstantaneousValue();

    sample *= amplitude * gain ;
    setBufferValue(0,sample) ;
}

double Oscillator::modulate([[maybe_unused]] double value, [[maybe_unused]] ModulationData* mdat ) const {
    return getCurrentSample(0) ;
}

void Oscillator::tick(){
    AudioSignalComponent::tick();

    auto frequency = parameters_->getParameter<ParameterType::FREQUENCY>()->getInstantaneousValue();
    auto detune = parameters_->getParameter<ParameterType::DETUNE>();
    if ( detune ){
        frequency *= dsp::getDetuneScale(detune->getInstantaneousValue());
    }

    increment_ = frequency / sampleRate_ ;
    phase_ = std::fmod(phase_ + increment_, 1.0);
}

void Oscillator::addReferenceParameters(ParameterMap& map){
    parameters_->addReferences(map);
}

void Oscillator::setWaveform(Waveform wave){
    parameters_->getParameter<ParameterType::WAVEFORM>()->setValue(wave) ;
}

void Oscillator::setFrequency(double freq){
    parameters_->getParameter<ParameterType::FREQUENCY>()->setValue(freq) ;
}

void Oscillator::setAmplitude(double amp){
    parameters_->getParameter<ParameterType::AMPLITUDE>()->setValue(amp);
}

