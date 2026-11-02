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

#include "capabilities/Parameters.hpp"

Parameters::Parameters(ParameterListener* listener):
    map_(),
    listener_(listener)
{}

void Parameters::markPerVoice(ParameterType p){ 
    map_.markPerVoice(p); 
}

void Parameters::listen(ParameterType p){
    if ( !listener_ ) return ;
    if ( auto* param = map_.getParameter(p) ) param->addListener(listener_);
    else if ( auto* c = map_.getCollection(p) ) c->addListener(listener_);
}

ParameterBase* Parameters::get(ParameterType p) const { 
    return map_.getParameter(p); 
} 

void Parameters::modulate(){ 
    map_.modulate(); 
}

ParameterMap& Parameters::map() { 
    return map_ ; 
}
const ParameterMap& Parameters::map() const { 
    return map_ ; 
}

// Modulation
ParamStatus Parameters::setModulation(ParameterType p, const ModulationSource* m, ModulationData d ){
    auto* param = map_.getParameter(p);
    if ( !param )                  return ParamStatus::NoSuchParameter ;
    if ( !m )                      return ParamStatus::NullSource ;
    if ( !param->isModulatable() ) return ParamStatus::NotModulatable ;
    param->setModulation(m, withRequiredDefaults(m, std::move(d)));
    return ParamStatus::Ok ;
}

ParamStatus Parameters::removeModulation(ParameterType p){
    auto* param = map_.getParameter(p);
    if ( !param ) return ParamStatus::NoSuchParameter ;
    param->removeModulation();
    return ParamStatus::Ok ;
}

ParamStatus Parameters::setDepthModulation(ParameterType p, const ModulationSource* m, ModulationData d ){
    auto* param = map_.getParameter(p);
    if ( !param ) return ParamStatus::NoSuchParameter ;

    auto* depth = param->getDepth();
    if ( !depth )                  return ParamStatus::NoDepth ;                
    if ( !m )                      return ParamStatus::NullSource ;
    if ( !depth->isModulatable() ) return ParamStatus::NotModulatable ;
    depth->setModulation(m, withRequiredDefaults(m, std::move(d)));
    return ParamStatus::Ok ;
}

ParamStatus Parameters::removeDepthModulation(ParameterType p){
    auto* param = map_.getParameter(p);
    if ( !param ) return ParamStatus::NoSuchParameter ;
    
    auto* depth = param->getDepth();
    if ( !depth ) return ParamStatus::NoDepth ;                

    depth->removeModulation();
    return ParamStatus::Ok ;
}

const ModulationSource* Parameters::getModulator(ParameterType p){
    auto* param = map_.getParameter(p);
    return param ? param->getModulator() : nullptr ;
}

const ModulationSource* Parameters::getDepthModulator(ParameterType p){
    auto* param = map_.getParameter(p);
    auto* depth = param ? param->getDepth() : nullptr ;
    return depth ? depth->getModulator() : nullptr ;
}

std::optional<double> Parameters::getDepth(ParameterType p) const {
    auto* param = map_.getParameter(p);
    auto* depth = param ? param->getDepth() : nullptr ;
    if ( !depth ) return std::nullopt ;
    return depth->getValue();
}

ParamStatus Parameters::setDepth(ParameterType p, double v){
    auto* param = map_.getParameter(p);
    if ( !param ) return ParamStatus::NoSuchParameter ;

    auto* depth = param->getDepth();
    if ( !depth ) return ParamStatus::NoDepth ;

    depth->setValue(v);
    return ParamStatus::Ok ;
}

std::optional<ModulationStrategy> Parameters::getStrategy(ParameterType p) const {
    auto* param = map_.getParameter(p);
    if ( !param ) return std::nullopt ;
    return param->getModulationStrategy();
}

ParamStatus Parameters::setStrategy(ParameterType p, ModulationStrategy s){
    auto* param = map_.getParameter(p);
    if ( !param ) return ParamStatus::NoSuchParameter ;

    param->setModulationStrategy(s);
    return ParamStatus::Ok ;
}

ModulationData Parameters::withRequiredDefaults(const ModulationSource* m, ModulationData d){
    if ( d.isEmpty() ){
        for ( auto mp : m->getRequiredModulationParameters() ){
            d.set(mp, 0.0f);
        }
    }
    return d ;
}

json Parameters::toJson(json& output) const { 
    return map_.toJson(output); 
}

void Parameters::fromJson(const json& j)    { 
    map_.fromJson(j); 
}
