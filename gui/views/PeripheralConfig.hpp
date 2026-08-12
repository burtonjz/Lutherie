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
    QLabel* audioSelectLabel_ ;
    QLabel* midiSelectLabel_ ;
    QComboBox* audioComboBox_ ;
    QComboBox* midiComboBox_ ;
    QCheckBox* audioPreferredCheck_ ;
    QCheckBox* midiPreferredCheck_ ;
    QPushButton* confirmButton_ ;

    bool audioDataReceived_ = false ;
    bool midiDataReceived_ = false ;
    bool autoSetupAttempted_ = false ;

    explicit PeripheralConfig(QWidget* parent = nullptr);
    static inline PeripheralConfig* instance_ = nullptr ;

public:
    static PeripheralConfig* instance();

    static void destroy();

    PeripheralConfig(const PeripheralConfig&) = delete ;
    PeripheralConfig& operator=(const PeripheralConfig&) = delete ;
    PeripheralConfig(PeripheralConfig&&) = delete ;
    PeripheralConfig& operator=(PeripheralConfig&&) = delete ;

    void setAudioDeviceId(int id, bool block = true);
    void setMidiDeviceId(int id, bool block = true);

private:
    void requestData();
    void populateComboBox(QComboBox* box, const json& data);
    void attemptAutoSetup();

private slots:
    void onControlMessageReceived(const json& json);
    void onConfigSubmit();

signals:
    void completed();
    void audioChannelsUpdated(size_t numChannels);
    void setupNeeded();

};

#endif // PERIPHERAL_CONFIG_HPP_
