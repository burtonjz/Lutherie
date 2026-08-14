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
#include "api/StreamingApiHandler.hpp"
#include <spdlog/spdlog.h>

Oscilloscope::Oscilloscope(ComponentId id, [[maybe_unused]] OscilloscopeConfig cfg):
    BaseComponent(id, ComponentType::Oscilloscope),
    AudioProbe(),
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

void Oscilloscope::onInputConnect(){
    collecting_ = true ;
}

void Oscilloscope::onInputDisconnect(){
    collecting_ = signalInputs_.size() > 0 ;
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
            it will not count as a candidate rising crossing. 
            
            We will find all candidate rising crossings and later determine which one is the most similar
            to the last buffer (in order to stabilize the output signal if multiple crossings exist)
            */
            double recentMin = *std::min_element(captureBuffer_.begin(), captureBuffer_.end());
            double recentMax = *std::max_element(captureBuffer_.begin(), captureBuffer_.end());
            double triggerLevel = ( recentMin + recentMax ) / 2.0 ;
            double hysteresis = ( recentMax - recentMin ) * hysteresisRatio_ ; 

            std::vector<double> candidates ;
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
                        // basic interpolation to get precise sample
                        double slope = captureBuffer_[j] - captureBuffer_[j-1];
                        double frac = (triggerLevel - captureBuffer_[j-1]) / slope ;
                        candidates.push_back(static_cast<double>(j) + frac);
                        armed = false ; // rearm to search for next candidate
                    }
                }
            }

            /*
            STEP 2: for each candidate, extract its window and score against
            the previously displayed window. Select the best match
            */
            bool triggered = false ;
            std::vector<float> bestWindow ;
            double bestScore = -2.0 ; // sentinal value, the score is bounded between -1 and 1
            double secondBestScore = -2.0 ; 
            [[maybe_unused]] double bestCandidatePos = 0.0 ;

            auto& prev = prevWindow_[id];

            for ( double candidate : candidates ){
                auto window = extractWindow(candidate);

                double score ;
                if ( prev.size() == windowSize_ ){
                    score = normalizedCrossCorrelation(window, prev);
                } else {
                    score = 0.0 ;
                }

                if ( score > bestScore ){
                    bestScore = score ;
                    bestWindow = std::move(window);
                    bestCandidatePos = candidate ;
                    triggered = true ;
                } else if ( score > secondBestScore ){
                    secondBestScore = score ;
                }
            }

            /*
            STEP 4: Send the window to the engine. If a trigger wasn't found, we will freeze up
            the output until we do trigger (the ui will fade this on its own)
            */
            if ( triggered ){
                [[maybe_unused]] double margin = bestScore - secondBestScore ;
                [[maybe_unused]] auto& lastPos = lastTriggerPos_[id];

                SPDLOG_TRACE(
                    "For componentId={}, oscilloscope has {} crossing candidates, chosen={:.1f} score={:.3f} margin={:.3f} deltaFromLast={:.1f}",
                    id, candidates.size(), bestCandidatePos, bestScore, margin, bestCandidatePos - lastPos
                );
                StreamingApiHandler::instance()->send(bestWindow, id);
                prevWindow_[id] = std::move(bestWindow);
            } 
            bufferPosition_ = 0 ;
        }
    }
}

std::vector<float> Oscilloscope::extractWindow(double triggerInterp){
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
    return window ;
}

double Oscilloscope::normalizedCrossCorrelation(const std::vector<float>& a, const std::vector<float>& b){
    /*
    This is a zero mean normalized cross-correlation, so DC offset won't affect the matching value
    Instead it just matches the "shape" of the curve
    */
    double meanA = std::accumulate(a.begin(), a.end(), 0.0) / a.size();
    double meanB = std::accumulate(b.begin(), b.end(), 0.0) / b.size();

    double num = 0.0, denomA = 0.0, denomB = 0.0 ;
    for ( size_t i = 0; i < a.size(); ++i ){
        double da = a[i] - meanA ;
        double db = b[i] - meanB ;
        num += da * db ;
        denomA += da * da ;
        denomB += db * db ;
    }

    if ( denomA <= 0.0 || denomB <= 0.0 ) return 0.0 ; // flat signal, don't divide by 0

    return num / std::sqrt(denomA * denomB);
}