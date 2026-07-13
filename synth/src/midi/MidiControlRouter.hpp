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

#ifndef MIDI_CONTROL_ROUTER_HPP_
#define MIDI_CONTROL_ROUTER_HPP_

#include "types/ParameterType.hpp"

#include <map>
#include <vector>

// forward declarations
class BaseComponent ;

class MidiControlRouter {
public:
    enum class ControlType {
        CONTINUOUS,
        DISCRETE
    };

private:
    struct Destination {
        BaseComponent* component ;
        ParameterType param ;

        Destination(BaseComponent* m, ParameterType p){
            component = m ;
            param = p ;
        }

        auto operator<=>(const Destination&) const = default ;
    };
    std::map<uint8_t, std::vector<Destination>> routes_ ;
    std::map<uint8_t, ControlType> controlTypes_ ;

    bool isLearning_ = false ;
    std::vector<std::pair<uint8_t, uint8_t>> learnEvents_ ;

    MidiControlRouter();

public:
    static MidiControlRouter* instance();
    MidiControlRouter(const MidiControlRouter&) = delete ;
    MidiControlRouter& operator=(const MidiControlRouter&) = delete ;
    MidiControlRouter(MidiControlRouter&&) = delete ;
    MidiControlRouter& operator=(MidiControlRouter&&) = delete ;

    uint8_t getControlForRoute(BaseComponent* component, ParameterType p) const ;
    bool registerRoute(uint8_t ctrl, BaseComponent* component, ParameterType p);
    void unregisterRoute(BaseComponent* component, ParameterType p);
    void setMidiControlType(uint8_t ctrl, ControlType typ);

    bool isLearning() const ;
    void setLearning(bool isLearning);

    void handleEvent(uint8_t ctrl, uint8_t value);

    json serialize() const ;

    static constexpr uint8_t INVALID_ROUTE = 128 ;
    static constexpr size_t NUM_LEARN_EVENTS_NEEDED = 3 ;

private:
    void handleLearnEvent(uint8_t ctrl, uint8_t value);
    void handleContinuousUpdate(uint8_t ctrl, uint8_t value);
    void handleDiscreteUpdate(uint8_t ctrl, uint8_t value);

    void notifyClients(int componentId, ParameterType p);

};

#endif // MIDI_CONTROL_ROUTER_HPP_