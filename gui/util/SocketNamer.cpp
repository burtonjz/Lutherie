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

#include "util/SocketNamer.hpp"
#include "managers/ComponentManager.hpp"
#include "meta/ComponentRegistry.hpp"

std::string SocketNamer::compute(const SocketSpec& spec) {
    std::string name = spec.type().toString();

    std::string idx = indexSuffix(spec);
    if ( idx != "" ){
        name += " " + idx ;
    }

    std::string comp = componentSuffix(spec.componentIds());
    if ( comp != "" ){
        name += " (" + comp + ")";
    }
        
    return name ;
}

std::string SocketNamer::componentSuffix(const std::set<int>& ids){
     auto* manager = ComponentManager::instance();

    if ( ids.size() == 1 ){
        return manager->getModel(*ids.begin())->getName().toStdString();
    }

    // if multiple 
    std::optional<ComponentType> commonType ;
    for ( int id : ids ){
        auto model = manager->getModel(id);
        if ( !commonType ){
            commonType = model->getType();
            continue ;
        } 
        
        if ( *commonType != model->getType() ){
            commonType = std::nullopt ;
            break ;
        }
    }

    if ( !commonType ){
        return fmt::format("{} Components", ids.size());
    } 
    
    return fmt::format(
        "{} {}s", 
        ids.size(), 
        ComponentRegistry::getComponentDescriptor(*commonType).name
    );
}

std::string SocketNamer::indexSuffix(const SocketSpec& spec){
    if ( spec.numEndpoints() > 1 ) return "" ;

    auto endpoints = spec.endpoints();
    if ( endpoints.size() != 1 || endpoints[0].index()) return "" ;

    return std::to_string(*endpoints[0].index() + 1);
}