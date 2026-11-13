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

#ifndef __COMPONENT_FACTORY_HPP_
#define __COMPONENT_FACTORY_HPP_

#include "core/ComponentManager.hpp"
#include "types/ComponentType.hpp"

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class ComponentFactory {
public:
    static int createFromJson(ComponentType type, const std::string& name, const json& j );
};

#endif // __COMPONENT_FACTORY_HPP_
