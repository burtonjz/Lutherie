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

#include "midi/MidiControlRouter.hpp"
#include "core/BaseComponent.hpp"
#include "api/ControlApiHandler.hpp"

#include <spdlog/spdlog.h>

MidiControlRouter::MidiControlRouter():
    routes_(),
    controlTypes_()
{}

MidiControlRouter* MidiControlRouter::instance(){
    static MidiControlRouter* s_instance = nullptr ;
    if ( !s_instance ){
        s_instance = new MidiControlRouter();
    }
    return s_instance ;
}

uint8_t MidiControlRouter::getControlForRoute(BaseComponent* component, ParameterType p) const {
    Destination dest(component, p);
    for ( auto [c, v] : routes_ ){
        auto it = std::find(v.begin(), v.end(), dest);
        if ( it != v.end() ){
            return c ;
        }
    }
    return INVALID_ROUTE ;
}

bool MidiControlRouter::registerRoute(uint8_t ctrl, BaseComponent* component, ParameterType p){
    if ( ctrl > 127 ){
        SPDLOG_ERROR(
            "Cannot handle midi control value of {}. Ignoring request", 
            ctrl
        );
        return false ;
    }

    if ( !component ){
        SPDLOG_ERROR(
            "Cannot create a midi control route for nullptr component"
        );
        return false ;
    }

    uint8_t existing = getControlForRoute(component, p);
    if ( existing != INVALID_ROUTE ){
        SPDLOG_ERROR(
            "Component with id {} already is routed to ctrl {}",
            component->getId(), existing
        );
        return false ;
    }

    if ( !controlTypes_.contains(ctrl) ){
        SPDLOG_ERROR(
            "Cannot route midi control {} to component parameter without"
            " a defined control type",
            ctrl
        );
        return false ;
    }

    Destination dest(component, p);
    routes_[ctrl].push_back(dest);
    return true ;
}

void MidiControlRouter::unregisterRoute(BaseComponent* component, ParameterType p){
    Destination dest(component, p);
    for ( auto [c, v] : routes_ ){
        auto it = std::find(v.begin(), v.end(), dest);
        if ( it != v.end() ){
            SPDLOG_DEBUG(
                "removing midi route from control value {} to parameter {}",
                c, GET_PARAMETER_TRAIT_MEMBER(dest.param, name)
            );
            v.erase(it);
        }
    }
}

void MidiControlRouter::setMidiControlType(uint8_t ctrl, ControlType typ){
    if ( ctrl > 127 ){
        SPDLOG_ERROR(
            "Cannot handle midi control value of {}. Ignoring request", 
            ctrl
        );
        return ;
    }

    controlTypes_[ctrl] = typ ;
}

bool MidiControlRouter::isLearning() const {
    return isLearning_ ;
}

void MidiControlRouter::setLearning(bool isLearning){
    isLearning_ = isLearning ;
}


void MidiControlRouter::handleEvent(uint8_t ctrl, uint8_t value){
    if ( ctrl > 127 ){
        SPDLOG_ERROR(
            "Cannot handle midi control value of {}. Ignoring request", 
            ctrl
        );
        return ;
    }

    if ( isLearning_ ){
        handleLearnEvent(ctrl, value);
        return ;
    }

    if ( !routes_.contains(ctrl) ) return ;

    if ( !controlTypes_.contains(ctrl) ){
        SPDLOG_ERROR(
            "Cannot handle midi route for ctrl value {}:"
            "control type is not defined",
            ctrl
        );
        return ;
    } 

    switch (controlTypes_.at(ctrl) ){
    case ControlType::CONTINUOUS:
        handleContinuousUpdate(ctrl, value);
        break ;
    case ControlType::DISCRETE:
        handleDiscreteUpdate(ctrl, value);
        break ;
    default:
        SPDLOG_ERROR("unrecognized ControlType received. exiting.");
        return ;
    }
}

void MidiControlRouter::handleLearnEvent(uint8_t ctrl, uint8_t value){
    // on first control touch, just set the value
    if ( learnEvents_.empty() ){
        learnEvents_.push_back({ctrl,value});
        return ;
    }

    // if new touch doesn't match last, reset and intake value
    const auto& lastEvent = learnEvents_.back();
    if ( ctrl != lastEvent.first ){
        learnEvents_.clear();
        learnEvents_.push_back({ctrl, value});
        return ;
    }

    // if it does match but we don't have enough data, just intake
    if ( learnEvents_.size() < NUM_LEARN_EVENTS_NEEDED ){
        learnEvents_.push_back({ctrl, value});
        return ;
    }

    // otherwise, send off learn result and end learn mode
    ControlType typ = ControlType::DISCRETE ;
    for ( const auto& [ctrl, val] : learnEvents_ ){
        if ( val != 127 && val != 0 ){
            typ = ControlType::CONTINUOUS ;
        }
    }

    json j ;
    j["action"] = "midi_learn" ;
    j["control_type"] = controlTypeToString(typ);
    j["value"] = ctrl ;
    controlTypes_[ctrl] = typ ;
    ControlApiHandler::instance()->handleClientMessage(j.dump());

    learnEvents_.clear();
    isLearning_ = false ;
}

void MidiControlRouter::handleContinuousUpdate(uint8_t ctrl, uint8_t value){
    double percent = value / 127.0 ;
    for ( const auto& d : routes_.at(ctrl) ){
        if ( !d.component ){
            continue ;
        }
        d.component->getParameters()->setValuePercentDispatch(d.param, percent);
        notifyClients(d.component->getId(), d.param);
    }
}

void MidiControlRouter::handleDiscreteUpdate(uint8_t ctrl, uint8_t value){
    // typically, discrete MIDI controls are 127 while pressed and 0 
    // when released so we want to trigger the update only on release
    if ( value != 0 ) return ;

    for ( const auto& d: routes_.at(ctrl) ){
        if ( !d.component ){
            continue ;
        }
        d.component->getParameters()->setValueTickWrapDispatch(d.param);
        notifyClients(d.component->getId(), d.param);
    }
}

void MidiControlRouter::notifyClients(int componentId, ParameterType p){
    json j ;
    j["action"] = "get_parameter" ;
    j["componentId"] = componentId;
    j["parameter"] = GET_PARAMETER_TRAIT_MEMBER(p, name);
    ControlApiHandler::instance()->handleClientMessage(j.dump());
}

json MidiControlRouter::serialize() const {
    json output ;
    for ( const auto& [ctrl, routes] : routes_ ){
        for ( const auto& route : routes ){
            if ( !route.component ) continue ;
            json j ;
            j["action"] = "set_midi_control" ;
            j["componentId"] = route.component->getId();
            j["parameter"] = GET_PARAMETER_TRAIT_MEMBER(
                route.param, 
                name
            );
            j["control_type"] = controlTypeToString(controlTypes_.at(ctrl)) ;        
            j["value"] = ctrl ;
            output.push_back(j);
        }
    }
    return output ;
}

const std::string& MidiControlRouter::controlTypeToString(ControlType typ){
    static const std::map<ControlType, std::string> c2s = {
        {ControlType::CONTINUOUS, "continuous"},
        {ControlType::DISCRETE, "discrete"}
    };
    return c2s.at(typ);
}

MidiControlRouter::ControlType MidiControlRouter::stringToControlType(const std::string& str){
    static const std::map<std::string, ControlType> s2c = {
        {"continuous", ControlType::CONTINUOUS},
        {"discrete", ControlType::DISCRETE}
    };
    if ( s2c.contains(str) ) return s2c.at(str) ;
    throw std::runtime_error(fmt::format("invalid string for ControlAction: {}", str));
}