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

#include "managers/StateManager.hpp"

StateManager::StateManager()
    : QObject{nullptr} 
{}

StateManager* StateManager::instance(){
    static StateManager manager ;
    return &manager ;
}

void StateManager::setSetupAudioComplete(bool v){
    setupAudio_ = v ;
    checkSetupConditions() ;
}

void StateManager::setSetupMidiComplete(bool v){
    setupMidi_ = v ;
    checkSetupConditions() ;
}

bool StateManager::isRunning() const  {
    return running_ ;
}

void StateManager::setRunning(bool v ){
    running_ = v ;
}

void StateManager::checkSetupConditions(){
    if ( setupAudio_ && setupMidi_ ){
        emit setupCompleted() ;
    }
}

