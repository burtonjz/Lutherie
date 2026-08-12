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

#include "views/PeripheralConfig.hpp"
#include "views/GraphPanel.hpp"
#include "managers/StateManager.hpp"
#include "api/ControlApiClient.hpp"
#include "views/GraphPanel.hpp"
#include "app/Theme.hpp"

#include <QBoxLayout>
#include <spdlog/spdlog.h>

PeripheralConfig* PeripheralConfig::instance(){
    if ( !instance_ ){
        instance_ = new PeripheralConfig();
    }
    return instance_ ;
}

void PeripheralConfig::destroy(){
    delete instance_ ;
    instance_ = nullptr ;
}

PeripheralConfig::PeripheralConfig(QWidget* parent): 
    QWidget(parent),
    audioSelectLabel_(new QLabel(Theme::SETUP_AUDIO_LABEL, this)),
    midiSelectLabel_(new QLabel(Theme::SETUP_MIDI_LABEL, this)),
    audioComboBox_(new QComboBox(this)),
    midiComboBox_(new QComboBox(this)),
    audioPreferredCheck_(new QCheckBox(Theme::SETUP_PREFERRED_LABEL, this)),
    midiPreferredCheck_(new QCheckBox(Theme::SETUP_PREFERRED_LABEL, this)),
    confirmButton_(new QPushButton(Theme::SETUP_CONFIRM_TEXT, this))
{
    // layout
    QGridLayout* layout = new QGridLayout(this);

    layout->addWidget(audioSelectLabel_, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(audioComboBox_, 0, 1);
    layout->addWidget(audioPreferredCheck_, 0, 2);

    layout->addWidget(midiSelectLabel_, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(midiComboBox_, 1, 1);
    layout->addWidget(midiPreferredCheck_, 1, 2);

    layout->addWidget(confirmButton_, 2, 1);

    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 0);
    layout->setRowStretch(3, 1);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setLayout(layout);
    
    // connections
    connect(
        ControlApiClient::instance(), &ControlApiClient::dataReceived, 
        this, &PeripheralConfig::onControlMessageReceived
    );
    connect(
        confirmButton_, &QPushButton::clicked,
        this, &PeripheralConfig::onConfigSubmit
    );
    connect(
        StateManager::instance(), &StateManager::setupCompleted, 
        this, &PeripheralConfig::completed
    );
    connect(
        this, &PeripheralConfig::audioChannelsUpdated,
        GraphPanel::instance(), &GraphPanel::updatePeripheralAudioChannels
    );

    // send initial device queries
    if ( ControlApiClient::instance()->isConnected() ){
        requestData();
    } else {
        connect(
            ControlApiClient::instance(), &ControlApiClient::connected,
            this, &PeripheralConfig::requestData, Qt::SingleShotConnection
        );
    }
    

}

void PeripheralConfig::setAudioDeviceId(int id, bool block){
    int idx = audioComboBox_->findData(id);
    if ( idx == -1 ){
        SPDLOG_ERROR("Audio peripheral device ID={} is not a recognized device.", id);
        return ;
    }

    audioComboBox_->setCurrentIndex(idx);
    if ( block ) return ;

    json j ;
    j["action"] = "set_audio_device" ;
    j["device_id"] = id ;
    ControlApiClient::instance()->sendMessage(j);

}

void PeripheralConfig::setMidiDeviceId(int id, bool block){
    int idx = midiComboBox_->findData(id);
    if ( idx == -1 ){
        SPDLOG_ERROR("MIDI peripheral device ID={} is not a recognized device.", id);
        return ;
    }

    midiComboBox_->setCurrentIndex(idx);
    if ( block ) return ;

    json j ;
    j["action"] = "set_midi_device" ;
    j["device_id"] = id ;
    ControlApiClient::instance()->sendMessage(j);
}

void PeripheralConfig::requestData(){
    json j ;
    j["action"] = "get_audio_devices" ;
    ControlApiClient::instance()->sendMessage(j);

    j["action"] = "get_midi_devices" ;
    ControlApiClient::instance()->sendMessage(j); 
}

void PeripheralConfig::populateComboBox(QComboBox* box, const json& data){
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

void PeripheralConfig::attemptAutoSetup(){
    SPDLOG_INFO("Attempting auto configuration from user config.");

    Config::load();
    bool complete = true ;

    int audioId = Config::get<int>("audio.preferred_device_id").value_or(-1);
    int midiId = Config::get<int>("midi.preferred_device_id").value_or(-1);

    if ( audioId != -1 ){
        int idx = audioComboBox_->findData(audioId);
        if ( idx == -1 ){
            complete = false ;
        } else {
            audioComboBox_->setCurrentIndex(idx);
            audioPreferredCheck_->setChecked(true);
        }
    } else {
        complete = false ;
    }

    if ( midiId != -1 ){
        int idx = midiComboBox_->findData(midiId);
        if ( idx == -1 ){
            complete = false ;
        } else {
            midiComboBox_->setCurrentIndex(idx);
            midiPreferredCheck_->setChecked(true);
        }
    } else {
        complete = false ;
    }

    if ( complete ){
        SPDLOG_INFO(
            "valid configuration recognized: audio device id = {}, midi device id = {}",
            audioId, midiId
        );
        onConfigSubmit();
    } else {
        SPDLOG_INFO(
            "user configuration is either incomplete or invalid. auto setup incomplete"
        );
        emit setupNeeded();
    }
}

void PeripheralConfig::onControlMessageReceived(const json& json){
    QString action = QString::fromStdString(json["action"]);

    if ( action == "get_audio_devices" ){
        populateComboBox(audioComboBox_, json.at("data"));
        audioDataReceived_ = true ;
        if ( audioDataReceived_ && midiDataReceived_ 
            && !autoSetupAttempted_ ) attemptAutoSetup();
        return ;
    }

    if ( action == "get_midi_devices" ){
        populateComboBox(midiComboBox_, json.at("data"));
        midiDataReceived_ = true ;
        if ( audioDataReceived_ && midiDataReceived_ 
            && !autoSetupAttempted_ ) attemptAutoSetup();
        return ;
    }

    if ( action == "set_audio_device" ){
        if ( json.at("status") == "success" ){
            setAudioDeviceId(json.at("device_id"));
            GraphPanel::instance()->updatePeripheralAudioChannels(json.at("output_channels"));
            StateManager::instance()->setSetupAudioComplete(true);
        }
        return ;
    }

    if ( action == "set_midi_device" ){
        if ( json.at("status") == "success" ){
            setMidiDeviceId(json.at("device_id"));
            StateManager::instance()->setSetupMidiComplete(true);
        }
        return ;
    }
}

void PeripheralConfig::onConfigSubmit(){
    json j ;
    j["action"] = "set_audio_device" ;
    j["device_id"] = audioComboBox_->currentData().toInt();
    j["preferred"] = audioPreferredCheck_->isChecked();
    ControlApiClient::instance()->sendMessage(j);

    j["action"] = "set_midi_device" ;
    j["device_id"] = midiComboBox_->currentData().toInt();
    j["preferred"] = midiPreferredCheck_->isChecked();
    ControlApiClient::instance()->sendMessage(j);
}
