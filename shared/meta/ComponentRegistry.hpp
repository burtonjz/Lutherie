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

#ifndef __SHARED_COMPONENT_REGISTRY_HPP_
#define __SHARED_COMPONENT_REGISTRY_HPP_

#include "meta/ComponentDescriptor.hpp"
#include <unordered_map>

namespace ComponentRegistry {

const ComponentDescriptor& getComponentDescriptor(ComponentType type);
const ComponentDescriptor& getComponentDescriptor(std::string componentName);

const std::unordered_map<ComponentType, ComponentDescriptor>& getAllComponentDescriptors();

};


#endif // __SHARED_COMPONENT_REGISTRY_HPP_