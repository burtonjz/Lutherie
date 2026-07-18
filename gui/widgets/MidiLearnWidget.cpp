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

#include "widgets/MidiLearnWidget.hpp"
#include "api/ControlApiClient.hpp"
#include "app/Theme.hpp"

#include <QLayout>
#include <spdlog/spdlog.h>

MidiLearnWidget::MidiLearnWidget(QWidget* parent):
    QWidget(parent),
    instructions_(new QLabel(Theme::getMidiLearnInstructions(), this)),
    resultMidi_(new QLabel("waiting for midi...", this)),
    confirmBtn_(new QPushButton("confirm", this)),
    restartBtn_(new QPushButton("reset", this)),
    ctrl_(128)
{
    QVBoxLayout* layout = new QVBoxLayout();
    QHBoxLayout* btnLayout = new QHBoxLayout();

    layout->addWidget(instructions_);
    layout->addWidget(resultMidi_);
    btnLayout->addWidget(confirmBtn_);
    btnLayout->addWidget(restartBtn_);
    layout->addLayout(btnLayout);
    setLayout(layout);

    resultMidi_->setText("Waiting for midi...");
    confirmBtn_->setDisabled(true);
    restartBtn_->setDisabled(true);

    connect(
        confirmBtn_, &QPushButton::pressed,
        this, &MidiLearnWidget::onConfirmButtonPressed
    );
    connect(
        restartBtn_, &QPushButton::pressed,
        this, &MidiLearnWidget::onRestartButtonPressed
    );
    connect(
        ControlApiClient::instance(), &ControlApiClient::dataReceived,
        this, &MidiLearnWidget::onControlMessageReceived
    );

    sendMidiLearnMessage();
}

void MidiLearnWidget::sendMidiLearnMessage(){
    json j ;
    j["action"] = "midi_learn" ;
    ControlApiClient::instance()->sendMessage(j);
}

void MidiLearnWidget::onControlMessageReceived(const json& msg){
    if ( 
        !msg.contains("action") || 
        msg.at("action") != "midi_learn" ||
        !msg.contains("value") ||
        !msg.contains("status") ||
        msg.at("status") != "success"
    ) return ;

    ctrl_ = msg.at("value");
    restartBtn_->setEnabled(true);
    confirmBtn_->setEnabled(true);
    resultMidi_->setText(QString("Midi Value: %1").arg(ctrl_));
}

void MidiLearnWidget::onRestartButtonPressed(){
    sendMidiLearnMessage();

    resultMidi_->setText("Waiting for midi...");
    confirmBtn_->setDisabled(true);
    restartBtn_->setDisabled(true);
}

void MidiLearnWidget::onConfirmButtonPressed(){
    emit midiControlSelected(ctrl_);
}