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

#ifndef __SIGNAL_CONTROLLER_HPP_
#define __SIGNAL_CONTROLLER_HPP_

#include "core/AudioSignalComponent.hpp"
#include "signal/SignalChain.hpp"

class SignalController {
private:
    SignalChain signalChain_ ;
    size_t numChannels_ ;
    std::vector<double> outputs_ ;

    SignalController();

public:
    static SignalController* instance();

    SignalController(const SignalController&) = delete ;
    SignalController& operator=(const SignalController&) = delete ;
    SignalController(SignalController&&) = delete ;
    SignalController& operator=(SignalController&&) = delete ;

    size_t getNumChannels() const ;
    void setNumChannels(size_t numChannels);

    void connect(AudioSignalComponent* from, size_t fromIndex, AudioSignalComponent* to, size_t toIndex);
    void disconnect(AudioSignalComponent* from, size_t fromIndex, AudioSignalComponent* to, size_t toIndex);

    /**
     * @brief the specified component's output is aggregated into the engine's output signal
     */
    void registerSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx);
    void unregisterSink(AudioSignalComponent* outbound, size_t outboundIdx, size_t inboundIdx);

    const SignalConnectionSet& getSinks(size_t channel) const ;

    std::pair<double*, size_t> processFrame();
    void clearBuffer();
    void updateProcessingGraph();
    void reset();
};

#endif // __SIGNAL_CONTROLLER_HPP_