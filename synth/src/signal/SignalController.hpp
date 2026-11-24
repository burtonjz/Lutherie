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

#include "core/AudioIO.hpp"
#include "requests/ConnectionRequest.hpp"
#include "signal/SignalChain.hpp"
#include "types/ConnectionEndpoint.hpp"

class SignalController {
private:
    SignalChain signalChain_ ;
    
    SignalController();

public:
    static SignalController* instance();

    SignalController(const SignalController&) = delete ;
    SignalController& operator=(const SignalController&) = delete ;
    SignalController(SignalController&&) = delete ;
    SignalController& operator=(SignalController&&) = delete ;

    void processSignalChain();
    
    void clearBuffer();
    void updateProcessingGraph();
    void reset();
    

    bool handleConnection(const ConnectionRequest& req);

    const AudioConnectionSet& getSinks(const ConnectionEndpoint& device) const ;
    const AudioConnectionSet& getSources(const ConnectionEndpoint& device) const ;

    void populateSources(const ConnectionEndpoint& device, double* data, size_t size);
    
private:
    bool connect(AudioIO* from, size_t fromIndex, AudioIO* to, size_t toIndex);
    bool disconnect(AudioIO* from, size_t fromIndex, AudioIO* to, size_t toIndex);

    bool registerSink(AudioIO* outbound, size_t outboundIdx, const ConnectionEndpoint& device);
    bool unregisterSink(AudioIO* outbound, size_t outboundIdx, const ConnectionEndpoint& device);

    bool registerSource(AudioIO* inbound, size_t inboundIdx, const ConnectionEndpoint& device);
    bool unregisterSource(AudioIO* inbound, size_t inboundIdx, const ConnectionEndpoint& device);
    
};

#endif // __SIGNAL_CONTROLLER_HPP_