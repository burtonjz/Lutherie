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

#include "Wavetable.hpp"
#include "config/Config.hpp"

#include <cmath>
#include <mutex>
#include <random>
#include <cstddef>

// define static members
std::once_flag Wavetable::initFlag_ ;
WaveMap Wavetable::waves_ ;

// define functions

const Wave Wavetable::getWavetable(Waveform waveform) {
    return { waves_[waveform].data(), waves_[waveform].size() } ;
}

void Wavetable::generate(){
    std::call_once(initFlag_, []() {
        Config::load();
        int s = Config::get<int>("oscillator.wavetable_size").value() ;
        for ( auto& v : waves_ ){
            v.resize(s);
        };

        generateSineWavetable();
        generateSquareWavetable();
        generateTriangleWavetable();
        generateSawWavetable();
        generateNoiseWavetable();
    });
}

void Wavetable::generateSineWavetable(){\
    std::vector<double>& w = waves_[Waveform::SINE];
    double phase ;

    for ( size_t i = 0; i < w.size() ; ++i ){
        phase = static_cast<double>(i) / w.size() ;
        w[i] = std::sin(2.0 * M_PI * phase);
    }
}

void Wavetable::generateSquareWavetable(){
    std::vector<double>& w = waves_[Waveform::SQUARE];

    double dt = 1.0 / w.size() ;
    double phase ;
    double sample ;

    for(size_t i = 0; i < w.size(); ++i){
        phase = static_cast<double>(i) / w.size() ;

        sample = phase < 0.5 ? 1.0 : -1.0 ;
        sample += polyBlep(phase,dt); // first drop
        sample -= polyBlep(std::fmod(phase + 0.5, 1.0), dt); // second drop

        w[i] = sample ;
    }
}

void Wavetable::generateTriangleWavetable(){
    std::vector<double>& w = waves_[Waveform::TRIANGLE];
    std::vector<double>& square = waves_[Waveform::SQUARE];

    // triangle is an integral of a square wave, we'll do a reimann sum
    double dt = 1.0 / w.size() ;
    double square_integral = -1.0;
    for(size_t i = 0; i < w.size(); ++i){
        square_integral += square[i] * dt * 4.0;
        w[i] = square_integral ;
    }
}

void Wavetable::generateSawWavetable(){
    std::vector<double>& w = waves_[Waveform::SAW];
    double sample ;
    double t ;
    double dt = 1.0 / w.size() ;

    for(size_t i = 0; i < w.size(); ++i){
        t = i * dt ;    
        sample = -1.0 + 2.0 * t ;
        sample -= polyBlep(t, dt);

        w[i] = sample ;
    }
}

void Wavetable::generateNoiseWavetable(){
    std::vector<double>& w = waves_[Waveform::NOISE];
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<double> distr(-1.0, 1.0);
    
    for(size_t i = 0; i < w.size(); ++i){
        w[i] = distr(rng);
    }
}

double Wavetable::polyBlep(double t, double dt){
    if (t < dt){
        t /= dt ;
        return t + t - t * t - 1.0 ;
    } 

    if (t > 1.0 - dt){
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }

    return 0.0;
}