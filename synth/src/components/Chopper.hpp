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

#ifndef CHOPPER_HPP_
#define CHOPPER_HPP_

#include "core/AudioBufferComponent.hpp"
#include "configs/ChopperConfig.hpp"

class Chopper : public AudioBufferComponent {
private:
    
public:
    Chopper(ComponentId id, ChopperConfig cfg);

protected:
    void onInputConnect() override ;
    void onInputDisconnect() override ;
    void onInputUpdated() override ;

    void onParameterChanged(ParameterType p) override ;

};

#endif // CHOPPER_HPP_