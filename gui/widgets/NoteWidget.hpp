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

#ifndef NOTE_WIDGET_HPP_
#define NOTE_WIDGET_HPP_

#include "types/SequenceData.hpp"

#include <QWidget>
#include <QLabel>
class NoteWidget : public QWidget {
    Q_OBJECT

private:
    uint8_t midiNote_ ;
    uint8_t velocity_ ;
    float startBeat_ ;
    float endBeat_ ;
    QString noteName_ ;
    bool selected_ ;

    float x_ ;
    float y_ ;
    float w_ ;

    QLabel* label_ ;
    
public:
    NoteWidget(uint8_t midiNote, uint8_t velocity, float start, float end, QWidget* parent = nullptr);
    NoteWidget(SequenceNote note, QWidget* parent = nullptr);

    void paintEvent(QPaintEvent*) override ;

    // Getters/Setters
    uint8_t getMidiNote() const ;
    void setMidiNote(uint8_t midiNote);
    uint8_t getVelocity() const ;
    void setVelocity(uint8_t velocity);
    float getStartBeat() const ;
    void setStartBeat(float startBeat, bool round = false);
    float getEndBeat() const ;
    void setEndBeat(float endBeat, bool round = false);
    void setBeatRange(float start, float end, bool round = false);

    void setSelected(bool selected);
    bool isSelected() const ;
    
    SequenceNote getNote() const ;

    QString midi2str(uint8_t note);

private:
    void updateSize();

};



#endif // NOTE_WIDGET_HPP_