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

#include "components/Oscilloscope.hpp"
#include "dsp/AnalyticsEngine.hpp"
#include <spdlog/spdlog.h>

Oscilloscope::Oscilloscope(ComponentId id, [[maybe_unused]] OscilloscopeConfig cfg):
    Analyzer(id, ComponentType::Oscilloscope),
    captureBuffer_(),
    bufferPosition_(0),
    sampleRate_(Config::get<double>("audio.sample_rate").value()),
    windowSize_(Config::get<int>("analysis.oscilloscope.window_size").value()),
    searchRegion_(Config::get<int>("analysis.oscilloscope.search_region").value()),
    captureSize_ (windowSize_ + searchRegion_),
    hysteresisRatio_(Config::get<double>("analysis.oscilloscope.hysteresis_ratio").value()),
    silenceThreshold_(Config::get<double>("analysis.oscilloscope.silence_threshold").value())
{
    captureBuffer_.resize(captureSize_);
}

void Oscilloscope::process(const double* data, size_t size, ComponentId id){
    /*
    An oscilloscope works by trying to "map" the source signal so that we can consistently display cycles
    in our viewer. The main trick is that we therefore need the outbound buffer to send starting from a 
    consistent point in the signal's cycle.
    */
    for ( size_t i = 0; i < size; ++i ){
        captureBuffer_[bufferPosition_++] = data[i];
        if ( bufferPosition_ >= captureSize_ ){
            /* 
            STEP 1: Now that we have captured enough data, we need to find a good trigger value. We're 
            going to pick a value that is likely to exist based on the range of values we have captured 
            thus far. It's also amplitude-agnostic, so works on quiet or loud signals.
            
            Hysteresis gives us a lag to the trigger to ensure we are actually crossing the value. That
            is, if we blip just under the threshold in our source signal and then cross over it again,
            it will not count as the rising crossing that we're looking for.
            */
            double recentMin = *std::min_element(captureBuffer_.begin(), captureBuffer_.end());
            double recentMax = *std::max_element(captureBuffer_.begin(), captureBuffer_.end());
            double triggerLevel = ( recentMin + recentMax ) / 2.0 ;
            double hysteresis = ( recentMax - recentMin ) * hysteresisRatio_ ; 

            /*
            STEP 2: as we continue to read through the source signal, we need to identify what sample we hit
            our trigger on. However, depending on the frequency/nature of the source signal, this may not 
            map cleanly to a whole number of samples. So we are going to interpolate to account for this.
            */
            bool triggered = false ;
            double triggerInterp = 0.0 ; 

            if ( (recentMax - recentMin) > silenceThreshold_ ){
                bool armed = false ;
                for ( size_t j = 1; j < searchRegion_; ++j ){
                    if ( !armed && captureBuffer_[j] < triggerLevel - hysteresis ){
                        armed = true ;
                    }
                    if ( 
                        armed && 
                        captureBuffer_[j-1] < triggerLevel && 
                        captureBuffer_[j] >= triggerLevel 
                    ){
                        double slope = captureBuffer_[j] - captureBuffer_[j-1];
                        double frac = (triggerLevel - captureBuffer_[j-1]) / slope ;
                        triggerInterp = static_cast<double>(j) + frac ;
                        triggered = true ;
                        break ;
                    }
                }
            } 

            /*
            STEP 3: extract the interpolated window from the captured buffer.
            */
            std::vector<float> window(windowSize_);
            for ( size_t j = 0; j < windowSize_; ++j ){
                double pos = triggerInterp + static_cast<double>(j);
                size_t idx = static_cast<size_t>(pos);
                double frac = pos - static_cast<double>(idx);
                if ( idx + 1 < captureSize_ ){
                    window[j] = static_cast<float>(
                        captureBuffer_[idx] * (1.0 - frac) + captureBuffer_[idx + 1] * frac 
                    );
                } else {
                    window[j] = static_cast<float>(captureBuffer_[idx]);
                }
            }

            /*
            STEP 4: Send the window to the engine. If a trigger wasn't found, we will freeze up
            the output until we do trigger (the ui will fade this on its own)
            */
            if ( triggered ){
                AnalyticsEngine::instance()->send(window, id);
            } 
            bufferPosition_ = 0 ;
        }
    }
}