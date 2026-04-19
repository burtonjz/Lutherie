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

#include "components/SpectrumAnalyzer.hpp"

#include <kissfft/kiss_fft.h>
#include <spdlog/spdlog.h>

SpectrumAnalyzer::SpectrumAnalyzer(ComponentId id, [[maybe_unused]] SpectrumAnalyzerConfig cfg):
    Analyzer(id, ComponentType::SpectrumAnalyzer),
    fftSize_(Config::get<unsigned int>("analysis.buffer_size").value_or(4096)),
    bufferPosition_(0)
{
    fftBuffer_.resize(fftSize_);
    fftConfig_ = kiss_fft_alloc(fftSize_, 0, nullptr, nullptr);
    if ( !fftConfig_ ){
        SPDLOG_ERROR("Failed to allocate KissFFT config");
    }

}

SpectrumAnalyzer::~SpectrumAnalyzer(){
    if ( fftConfig_ ){
        kiss_fft_free(fftConfig_);
        fftConfig_ = nullptr ;
    }
}

void SpectrumAnalyzer::process(const double* data, size_t size, ComponentId id){
    if ( !fftConfig_ ) return ;

    for ( size_t i = 0 ; i < size ; ++i ){
        fftBuffer_[bufferPosition_++] = data[i];

        if ( bufferPosition_ >= fftSize_ ){
            // apply Hann window
            std::vector<double> windowedData = fftBuffer_ ;
            for ( size_t j = 0; j < fftSize_; ++j ) {
                double window = 0.5 * (1.0 - cos(2.0 * M_PI * j / (fftSize_ - 1)));
                windowedData[j] *= window ;
            }

            // Run FFT
            std::vector<kiss_fft_cpx> fftInput(fftSize_);
            std::vector<kiss_fft_cpx> fftOutput(fftSize_);

            for (size_t j = 0; j < fftSize_; ++j){
                fftInput[j].r = windowedData[j];
                fftInput[j].i = 0.0;
            }

            kiss_fft(fftConfig_, fftInput.data(), fftOutput.data());

            // calculate magnitudes
            std::vector<float> magnitudes(fftSize_ / 2);
            for (size_t j = 0; j < fftSize_ / 2; ++j){
                kiss_fft_scalar real = fftOutput[j].r;
                kiss_fft_scalar imag = fftOutput[j].i;
                float magnitude = std::sqrt(real * real + imag * imag);
                magnitude = std::max(magnitude, 1e-10f) / (fftSize_ / 2.0);
                magnitudes[j] = 20.0 * std::log10(magnitude);
            }

            AnalyticsEngine::instance()->send(magnitudes, id);

            // 50% overlap
            std::memmove(fftBuffer_.data(),
                fftBuffer_.data() + fftSize_ / 2,
                (fftSize_ / 2) * sizeof(double)
            );
            bufferPosition_ = fftSize_ / 2 ;
        }
    }
}