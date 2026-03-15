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
#include "app/ModuleContext.hpp"
#include "api/ApiClient.hpp"
#include "ui_Setup.h"

Setup::Setup(ModuleContext ctx, QWidget* parent): 
    QWidget(parent),
    ui_(new Ui::AudioMidiSetupWidget),
    ctx_(ctx)
{
    // create API connections
    connect(ApiClient::instance(), &ApiClient::dataReceived, this, &Setup::onApiDataReceived);

    // ask backend for relevant data
    json j ;
    j["action"] = "get_audio_devices" ;
    ApiClient::instance()->sendMessage(j);
    j["action"] = "get_midi_devices" ;
    ApiClient::instance()->sendMessage(j); 

    ui_->setupUi(this);

    // connections
    connect(ui_->pushButtonSubmit, &QPushButton::clicked, this, &Setup::onSetupSubmit);
    connect(ctx_.state, &StateManager::setupCompleted, this, &Setup::onSetupCompleted);
}

Setup::~Setup()
{
    delete ui_;
}

void Setup::populateSetupComboBox(QComboBox* box, const json& data){
    if ( !data.is_array() ){
        qWarning() << "data is not in expected format. Exiting." ;
        return ;
    }

    box->clear();
    for ( const auto& item : data ){
        if ( ! item.is_array()  ){
            qWarning() << "Expected array elements to be arrays" ;
            continue ;
        }

        int deviceID = item.at(0);
        QString deviceName = QString::fromStdString(item.at(1)) ;
        QString displayText = QString("(%1) %2").arg(deviceID).arg(deviceName);
        box->addItem(displayText, deviceID);
    }
}


void Setup::onApiDataReceived(const json& json){
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
        qDebug() << "set_audio_device return state:" << status ;
        if ( status == "success" ){
            ctx_.state->setSetupAudioComplete(true);
        }
        return ;
    }

    if ( action == "set_midi_device" ){
        QString status = QString::fromStdString(json["status"]);
        qDebug() << "set_midi_device return state:" << status ;
        if ( status == "success" ){
            ctx_.state->setSetupMidiComplete(true);
        }
        return ;
    }
}

void Setup::onSetupSubmit(){
    qInfo() << "Setup submit button clicked." ;
    json j ;
    j["action"] = "set_audio_device" ;
    j["device_id"] = ui_->comboAudioDevice->currentData().toInt();
    ApiClient::instance()->sendMessage(j);

    j["action"] = "set_midi_device" ;
    j["device_id"] = ui_->comboMidiDevice->currentData().toInt();
    ApiClient::instance()->sendMessage(j);
}

void Setup::onSetupCompleted(){
    qInfo() << "setup completed." ;
    json j ; 
    emit setupCompleted();
    close();
}
