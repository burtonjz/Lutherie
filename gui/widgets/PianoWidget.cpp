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

#include "widgets/PianoWidget.hpp"
#include "app/Theme.hpp"

#include <QPainter>

PianoWidget::PianoWidget(QWidget* parent):
    QWidget(parent),
    keyWidth_(Theme::PIANO_KEY_LENGTH),
    keyHeight_(Theme::PIANO_KEY_THICKNESS),
    textPadW_(Theme::PIANO_KEY_LABEL_PAD),
    textPadH_(0),
    pianoWidth_(keyWidth_),
    pianoHeight_(keyHeight_ * 128) 
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    update();
}

bool PianoWidget::isWhiteNote(uint8_t pitch) const {
    uint8_t note = pitch % 12 ;
    return note == 0 || note == 2 || 
           note == 4 || note == 5 || 
           note == 7 || note == 9 || 
           note == 11 ;
}

QRect PianoWidget::getNoteDimensions(uint8_t pitch) const {
    int pos = static_cast<int>((127 - pitch) * Theme::PIANO_KEY_THICKNESS);
    if ( isVertical_ ){
        return {
            0, pos,
            keyWidth_, keyHeight_ 
        };
    } else {
        return {
            pos, 0,
            keyWidth_, keyHeight_
        };
    }
}

void PianoWidget::paintEvent(QPaintEvent*){
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    for ( uint8_t note = 0; note < 128; ++note ){
        int x, y, xPad, yPad ;
        if ( isVertical_ ){
            x = 0 ;
            xPad = Theme::PIANO_KEY_LABEL_PAD / 2 ;
            y = yPad = (127 - note) * Theme::PIANO_KEY_THICKNESS ;
        } else {
            x = xPad = (127 - note) * Theme::PIANO_KEY_THICKNESS ;
            y = 0 ;
            yPad = Theme::PIANO_KEY_LABEL_PAD / 2 ;
        }

        QColor keyColor ;
        if ( isWhiteNote(note) ){
            keyColor = Theme::PIANO_ROLL_KEY_WHITE ;
        } else {
            keyColor = Theme::PIANO_ROLL_KEY_BLACK ;
        }
        p.fillRect(x,y,keyWidth_, keyHeight_, keyColor);
        p.setPen(Theme::PIANO_ROLL_KEY_BORDER);
        p.drawRect(x,y,keyWidth_, keyHeight_);

        // draw some note names
        if ( note % 12 == 0 ){
            p.setPen(Theme::PIANO_ROLL_KEY_LABEL);
            p.drawText(
                QRect(
                    xPad,yPad, 
                    keyWidth_ - textPadW_, 
                    keyHeight_ - textPadH_
                ),
                Qt::AlignCenter,
                QString("C%1").arg(note / 12 - 1)
            );
        }
    }   
}

QSize PianoWidget::sizeHint() const {
    return { pianoWidth_, pianoHeight_ };
}

bool PianoWidget::isVertical() const {
    return isVertical_ ;
}

void PianoWidget::setVertical(bool vertical){
    isVertical_ = vertical ;
    if ( isVertical_ ){
        keyWidth_ = Theme::PIANO_KEY_LENGTH ;
        keyHeight_ = Theme::PIANO_KEY_THICKNESS ;
        textPadW_ = Theme::PIANO_KEY_LABEL_PAD ;
        textPadH_ = 0 ;
        pianoWidth_ = keyWidth_ ;
        pianoHeight_ = keyHeight_ * 128 ;
    } else {
        keyWidth_ = Theme::PIANO_KEY_THICKNESS ;
        keyHeight_ = Theme::PIANO_KEY_LENGTH ;
        textPadW_ = 0 ;
        textPadH_ = Theme::PIANO_KEY_LABEL_PAD ;
        pianoWidth_ = keyWidth_ * 128 ;
        pianoHeight_ = keyHeight_ ;
    }

    update();
}
