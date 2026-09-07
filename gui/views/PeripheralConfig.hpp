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

#ifndef PERIPHERAL_CONFIG_HPP_
#define PERIPHERAL_CONFIG_HPP_

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class PeripheralConfig : public QWidget {
    Q_OBJECT

private:
    QComboBox* audioComboBox_ ;
    QComboBox* midiComboBox_ ;
    QCheckBox* audioPreferredCheck_ ;
    QCheckBox* midiPreferredCheck_ ;

    bool audioDataReceived_ = false ;
    bool midiDataReceived_ = false ;
    bool autoSetupAttempted_ = false ;

public:
    explicit PeripheralConfig(QWidget* parent = nullptr);

    void setAudioDeviceId(int id, bool block = true);
    void setMidiDeviceId(int id, bool block = true);

    void submit();

private:
    void requestData();
    void populateComboBox(QComboBox* box, const json& data);
    void attemptAutoSetup();

private slots:
    void onControlMessageReceived(const json& json);

signals:
    void accept();
    void reject();
    void setupNeeded();

};

#endif // PERIPHERAL_CONFIG_HPP_
