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

#ifndef MIDI_LEARN_WIDGET_HPP_
#define MIDI_LEARN_WIDGET_HPP_

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::json ;

class MidiLearnWidget : public QWidget {
    Q_OBJECT

private:
    QLabel* instructions_ ;
    QLabel* resultMidi_ ;
    QPushButton* confirmBtn_ ;
    QPushButton* restartBtn_ ;
    uint8_t ctrl_ ;

public:
    explicit MidiLearnWidget(QWidget* parent = nullptr);

private:
    void sendMidiLearnMessage();
    
public slots:
    void onControlMessageReceived(const json& msg); // handle midi learn events directly
    void onRestartButtonPressed();
    void onConfirmButtonPressed();

signals:
    void midiControlSelected(uint8_t ctrl);

};

#endif // MIDI_LEARN_WIDGET_HPP_