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

#include "app/Setup.hpp"
#include "managers/StateManager.hpp"
#include "api/ControlApiClient.hpp"
#include "ui_Setup.h"
#include <spdlog/spdlog.h>

Setup::Setup(QWidget* parent): 
    QWidget(parent),
    ui_(new Ui::AudioMidiSetupWidget)
{
    // create API connections
    connect(ControlApiClient::instance(), &ControlApiClient::dataReceived, this, &Setup::onControlMessageReceived);

    // ask backend for relevant data
    json j ;
    j["action"] = "get_audio_devices" ;
    ControlApiClient::instance()->sendMessage(j);
    j["action"] = "get_midi_devices" ;
    ControlApiClient::instance()->sendMessage(j); 

    ui_->setupUi(this);

    // connections
    connect(ui_->pushButtonSubmit, &QPushButton::clicked, this, &Setup::onSetupSubmit);
    connect(StateManager::instance(), &StateManager::setupCompleted, this, &Setup::onSetupCompleted);
}

Setup::~Setup()
{
    delete ui_;
}

void Setup::populateSetupComboBox(QComboBox* box, const json& data){
    if ( !data.is_array() ){
        SPDLOG_WARN("data is not in expected format. Exiting.");
        return ;
    }

    box->clear();
    for ( const auto& item : data ){
        if ( ! item.is_object() || ! item.contains("id") || ! item.contains("name") ){
            SPDLOG_WARN("array object does not match expected form. Skipping item: {}", item.dump()) ;
            continue ;
        }

        int deviceID = item.at("id");
        QString deviceName = QString::fromStdString(item.at("name")) ;
        QString displayText = QString("(%1) %2").arg(deviceID).arg(deviceName);
        box->addItem(displayText, deviceID);
    }
}


void Setup::onControlMessageReceived(const json& json){
    QString action = QString::fromStdString(json["action"]);

    if ( action == "get_audio_devices" ){
        populateSetupComboBox(ui_->comboAudioDevice, json.at("data"));
        return ;
    }

    if ( action == "get_midi_devices" ){
        populateSetupComboBox(ui_->comboMidiDevice, json.at("data"));
        return ;
    }

    if ( action == "set_audio_device" ){
        QString status = QString::fromStdString(json["status"]);
        SPDLOG_DEBUG("set_audio_device return state: {}", status.toStdString());
        if ( status == "success" ){
            StateManager::instance()->setSetupAudioComplete(true);
        }
        return ;
    }

    if ( action == "set_midi_device" ){
        QString status = QString::fromStdString(json["status"]);
        SPDLOG_DEBUG("set_midi_device return state: {}", 
            status.toStdString());
        if ( status == "success" ){
            StateManager::instance()->setSetupMidiComplete(true);
        }
        return ;
    }
}

void Setup::onSetupSubmit(){
    json j ;
    j["action"] = "set_audio_device" ;
    j["device_id"] = ui_->comboAudioDevice->currentData().toInt();
    ControlApiClient::instance()->sendMessage(j);

    j["action"] = "set_midi_device" ;
    j["device_id"] = ui_->comboMidiDevice->currentData().toInt();
    ControlApiClient::instance()->sendMessage(j);
}

void Setup::onSetupCompleted(){
    SPDLOG_INFO("setup completed.");
    json j ; 
    emit setupCompleted();
    close();
}
